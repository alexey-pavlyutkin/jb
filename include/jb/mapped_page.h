#include "aligned_atomic.h"
#include <boost/smart_ptr/intrusive_ptr.hpp>


namespace jb
{
    namespace details
    {
        template < typename Policies >
        class storage_file< Policies >::cache::mapped_page
        {
            friend class cache;

            using safe_mapped_area = typename storage_file::safe_mapped_area;

            storage_file& file_;
            size_t offset_ = 0;
            aligned_atomic< uintptr_t > next_ = nullptr;
            aligned_atomic< size_t > ref_count_ = 0;
            aligned_atomic< int > lock_count_ = -1;
            safe_mapped_area mapping_;

            static constexpr int unlocked_ = -1;
            static constexpr int under_locking_ = 0;
            static constexpr int once_locked_ = 1;

        public:

            using mapped_page_ptr = boost::intrusive_ptr< mapped_page >;

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

            void* data() const
            {
                return nullptr;
            }
        };

    }
}