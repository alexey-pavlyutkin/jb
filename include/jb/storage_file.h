#ifndef __JB__STORAGE_FILE__H__
#define __JB__STORAGE_FILE__H__


#include "ret_codes.h"
#include "exception.h"
#include "aligned_atomic.h"
#include <filesystem>
#include <thread>
#include <boost/smart_ptr/intrusive_ptr.hpp>


namespace jb
{
    namespace details
    {
        template < typename Policies >
        class storage_file : public Policies::api
        {
            struct mapped_page;
            using mapped_page_ptr = typename mapped_page::mapped_page_ptr;

            std::vector< mapped_page > cache_;
            aligned_atomic< uintptr_t > unused_pages_;
            std::vector< aligned_atomic< uintptr_t > > mapped_pages_;
            size_t bucket_count_;
            static constexpr uintptr_t owned_ = 1;

        public:

            storage_file() = delete;
            storage_file( storage_file&& ) = delete;

            explicit storage_file( std::filesystem::path&& path ) try
                : api( std::filesystem::absolute( path ) )
            {
                if ( !size() ) grow();
            }
            catch ( const std::filesystem::filesystem_error & e )
            {
                throw runtime_error( RetCode::InvalidFilePath, "Invalid file path" );
            }

            mapped_page_ptr get_mapped_page( size_t offset )
            {
                assert( mapped_pages_.size() );
                auto bucket = offset / page_size() % mapped_pages_.size();

                aligned_atomic< uintptr_t >* p_current = &used_pages_[ bucket ];
                aligned_atomic< uintptr_t >* p_previous = nullptr; 
                uintptr_t current;
                uintptr_t previous;

                while ( true )
                {
                    // try get ownership over current item in the bucket
                    for ( size_t spin = 1; ; ++spin )
                    {
                        if ( current = p_current->fetch_or( owned_, std::memory_order_acq_rel ), !( n & owned_ ) )
                        {
                            // got the item
                            break;
                        }

                        // all the attempts failed - fall asleep for a while
                        static constexpr size_t spin_count = 1024;
                        if ( spin % spin_count == 0 ) std::this_thread::yield();
                    }

                    // still we've already got ownership over the current item, the previous one can be released for other threads
                    if ( p_previous ) p_previous->store( previous, std::memory_order_release );

                    // get pointer to current page
                    auto p_page = reinterpret_cast< mapped_page* >( current );

                    // if page exists with offset that we're looking for
                    if ( p_page && p_page->offset_ == offset )
                    {
                        // requested page found: make up result and release ownership
                        mapped_page_ptr page( p_page );
                        p_current->store( current, std::memory_order_release );
                        return page;
                    }
                    // if we've reached end of the bucked or met a page with greater offset
                    else if ( !p_page || p_page->offset_ > offset )
                    {
                        // requested page not mapped - try to map 
                        while ( true )
                        {
                            // get the 1st unused page
                            auto first_unused = unused_pages_.load( std::memory_order_acquire );
                            if ( !first_unused ) throw runtime_error( RetCode::Overloaded, "Volume cache exceeded the limit" );
                            p_page = reinterpret_cast< mapped_page* >( first_unused );

                            // remove the page from "unused" list
                            auto next_unused = p_page->next_.load( std::memory_order_acquire );
                            if ( !unused_pages_.compare_exchange_weak( unused, next_unused, std::memory_order_acq_rel, std::memory_order_relaxed ) )
                            {
                                continue;
                            }

                            // make up the page and insert it into the bucked at current position and release ownership
                            mapped_page_ptr page( reinterpret_cast< mapped_page* >( unused ), false );
                            page->offset_ = offset;
                            page->mapping_ = safe_mapped_area{};
                            page->ref_count_.store( 1, std::memory_order_relaxed );
                            page->lock_count_.store( mapped_page::unlocked_, std::memory_order_relaxed );
                            page->next_.store( current, std::memory_order_relaxed );
                            p_current->store( unused, std::memory_order_release );
                            return page;
                        }
                    }
                    else
                    {
                        // keep searching forward
                        p_previous = p_current;
                        previous = current;
                        p_current = &p_page->next_;
                    }
                }
            }

            void mark_page_as_unused( mapped_page* page )
            {
                assert( page );

                auto bucket = ( page->offset_ / page_size() ) % mapped_pages_.size();

                aligned_atomic< uintptr_t >* p_current = &used_pages_[ bucket ];
                aligned_atomic< uintptr_t >* p_previous = nullptr;
                uintptr_t current;
                uintptr_t previous;

                while ( true )
                {
                    // try get ownership over current item in the bucket
                    for ( size_t spin = 1; ; ++spin )
                    {
                        if ( current = p_current->fetch_or( owned_, std::memory_order_acq_rel ), !( n& owned_ ) )
                        {
                            // got the item
                            break;
                        }

                        // all the attempts failed - fall asleep for a while
                        static constexpr size_t spin_count = 1024;
                        if ( spin % spin_count == 0 ) std::this_thread::yield();
                    }

                    // still we've already got ownership over the current item, the previous one can be released for other threads
                    if ( p_previous ) p_previous->store( previous, std::memory_order_release );

                    // get pointer to current page
                    auto p_page = reinterpret_cast< mapped_page* >( current );
                    assert( p_page );
                    
                    //
                    // if page found and it's not referred (as soon as we've got the item owned with ACQUIRE
                    // semantic now we can use RELAXED to check reference count)
                    //
                    if ( p_page == page && !p_page->ref_count_.load( std::memory_order_relaxed ) )
                    {
                        // get the next item owned
                        for ( size_t spin = 1;; ++spin )
                        {
                            if ( auto next = p_page.load( std::memory_order_acquire ); !( next & hazard_ ) )
                            {
                                // and set current to next, removing the page from the bucket
                                p_current.store( next, std::memory_order_release );
                                break;
                            }

                            static constexpr size_t spin_count = 1024;
                            if ( !( spin % spin_count ) ) std::this_thread::yield();
                        }

                        // put the page in the front of unused list
                        while ( true )
                        {
                            auto unused_pages = unused_pages_.load( std::memory_order_acquire );
                            p_page->next_.store( unused_pages, std::memory_order_relaxed );
                            if ( unused_pages_.compare_exchange_weak( unused_pages, current, std::memory_order_acq_rel, std::memory_order_relaxed ) )
                            {
                                break;
                            }
                        }
                    }
                    else
                    {
                        // keep searching forward
                        p_previous = p_current;
                        previous = current;
                        p_current = &p_page->next_;
                    }
                }
            }
        };

        template < typename Policies >
        struct storage_file< Policies >::mapped_page
        {
            using mapped_page_ptr = boost::intrusive_ptr< mapped_page >;
            using safe_mapped_area = typename storage_file::safe_mapped_area;

            storage_file& file_;
            size_t offset_ = 0;
            aligned_atomic< uintptr_t > next_ = nullptr;
            aligned_atomic< size_t > ref_count_ = 0;
            aligned_atomic< int > lock_count_ = -1;
            safe_mapped_area mapping_;

            void intrusive_ptr_add_ref( mapped_page* page )
            {
                assert( page );
                page->ref_count_.fetch_add( 1 );
            }

            void intrusive_ptr_release( mapped_page* page )
            {
                assert( page );
                if ( 1 == page->ref_count_.fetch_sub( 1 ) )
                {
                    page->file_.mark_page_as_unused( page );
                }
            }

            static constexpr int unlocked_ = -1;
            static constexpr int under_locking_ = 0;
            static constexpr int once_locked_ = 1;

            void lock()
            {
                if ( auto lock_count = lock_count_.fetch_add( 1, std::memory_order_acq_rel ); unlocked_ == lock_count )
                {
                    mapping_ = std::move( file_.map_page( offset_ ) );
                    lock_count_.store( once_locked_, std::memory_order_release );
                }
                else for ( size_t spin = 1; lock_count == under_locking_; ++spin )
                {
                    static constexpr size_t spin_count = 1 << 16;
                    if ( spin % spin_count == 0 ) std::this_thread::yield();
                }
            }

            void unlock() noexcept
            {
                if ( once_locked_ == lock_count_.fetch_sub( 1, std::memory_order_acq_rel ) )
                {
                    data_ = nullptr;
                    mapping_.swap( safe_mapped_area{} );
                    lock_count_.store( unlocked_ );
                }
            }

            void * data() const
            {
                return mapping_
            }
        };
    }
}

#endif
