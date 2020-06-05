#ifndef __JB__CACHE__H__
#define __JB__CACHE__H__


#include "aligned_atomic.h"
#include <memory_resource>
#include <thread>


namespace jb
{
    namespace details
    {
        template < typename Policies >
        class storage_file< Policies >::cache
        {

        public:

            class mapped_page;

        private:

            static constexpr size_t bucket_count_ = 41;
            static constexpr uintptr_t owned_ = 1;

            storage_file& file_;
            std::pmr::monotonic_buffer_resource monotonic_buffer_;
            std::pmr::polymorphic_allocator< mapped_page > allocator_;
            std::array< aligned_atomic< uintptr_t >, bucket_count_ > used_pages_;
            aligned_atomic< uintptr_t > unused_pages_;
            aligned_atomic< size_t > size_, used_;

        public:

            class mapped_page;
            using mapped_page_ptr = typename mapped_page::mapped_page_ptr;

            cache() = delete;
            cache( cache&& ) = delete;

            explicit cache( storage_file& file )
                : file_( file )
                , monotonic_buffer_()
                , allocator_( &monotonic_buffer_ )
            {}

            size_t size() const noexcept { return size_.load( std::memory_order_acquire ); }
            size_t used() const noexcept { return used_.load( std::memory_order_acquire ); }

            template < typename... Args >
            mapped_page* new_page( Args&&... args )
            {
                try
                {
                    auto p = allocator_->allocate( 1 );
                    assert( p );
                    //
                    allocator_->construct( p, std::forward< Args >( args )... );
                    //
                    return p
                }
                catch ( ... )
                {
                    throw std::bad_alloc();
                }
            }

            mapped_page_ptr get_mapped_page( size_t offset )
            {
                auto bucket = ( offset / file_.page_size() ) % bucket_count_;

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
                    auto p_page = reinterpret_cast<mapped_page*>( current );

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
                            p_page = reinterpret_cast<mapped_page*>( first_unused );
                            if ( p_page )
                            {
                                // remove the page from "unused" list
                                auto next_unused = p_page->next_.load( std::memory_order_acquire );
                                if ( !unused_pages_.compare_exchange_weak( unused, next_unused, std::memory_order_acq_rel, std::memory_order_relaxed ) )
                                {
                                    continue;
                                }
                            }
                            else
                            {
                                p_page = new_page( file_, *this );
                            }

                            // make up the page and insert it into the bucked at current position and release ownership
                            mapped_page_ptr page( p_page, false );
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

            bool try_release_mapped_page( mapped_page* page ) noexcept
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
                    auto p_page = reinterpret_cast<mapped_page*>( current );
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
    }
}

#include "mapped_page.h"

#endif
