#ifndef __JB__MAPPED_REGION__H__
#define __JB__MAPPED_REGION__H__


#include "exception.h"
#include <boost/iostreams/device/mapped_file.hpp>
#include <atomic>
#include <shared_mutex>
#include <new>
#include <filesystem>
#include <string>
#include <assert.h>


namespace jb
{
    namespace details
    {
        template< typename SharedMutex >
        class mapped_region
        {
            const int alignment_ = boost::iostreams::mapped_file::alignment();

            SharedMutex& resize_guard_;
            std::shared_lock< SharedMutex > resize_lock_;
            const std::string & path_;
            const int64_t offset_;
            const size_t size_;
            boost::iostreams::mapped_file file_;
            alignas( std::hardware_destructive_interference_size ) std::atomic_int lock_count_;

        public:

            mapped_region() = delete;
            mapped_region( mapped_region&& ) = delete;

            mapped_region( SharedMutex & resize_guard, const std::string& path, int64_t offset, size_t size = 0 ) noexcept
                : resize_guard_( resize_guard )
                , path_( path )
                , offset_( offset )
                , size_( size ? size : alignment_ )
                , lock_count_( 0 )
            {
                assert( offset_ % alignment_ == 0 );
            }

            ~mapped_region()
            {
                if ( file_.is_open() ) file_.close();
            }

            const std::string & path() const noexcept { return path_; }
            int64_t offset() const noexcept { return offset_; }
            size_t size() const noexcept { return size_; }
            
            bool is_open() const noexcept
            {
                try
                {
                    return file_.is_open();
                }
                catch ( ... )
                {
                    return false;
                }
            }

            void lock() noexcept
            {
                try
                {
                    auto lock_count = lock_count_.fetch_add( 1, std::memory_order_acq_rel );

                    if ( !lock_count )
                    {
                        // take shared lock over the file to diable resizing
                        std::shared_lock< SharedMutex > resize_lock( resize_guard_ );

                        // open the file
                        file_.open( path_, boost::iostreams::mapped_file_base::readwrite, size_, offset_ );

                        // preserve the lock
                        resize_lock_ = std::move( resize_lock );
                    }
                }
                catch ( ... )
                {
                    assert( false );
                }
            }


            /** Decrease lock count, and if there is no locks anymore
            */
            void unlock() noexcept
            {
                try
                {
                    auto lock_count = lock_count_.fetch_sub( 1, std::memory_order_acq_rel );
                    assert( lock_count > 0 );

                    if ( 1 == lock_count )
                    {
                        assert( file_.is_open() );

                        // close the file
                        file_.close();

                        // release shared lock and enable resizing
                        std::shared_lock< SharedMutex > resize_lock( std::move( resize_lock_ ) );
                    }
                }
                catch ( ... )
                {
                    assert( false );
                }
            }

            /** Provides access to mapped region memory space

            @retval
            @throw runtime_error if there is an error
            */
            char* data() const
            {
                assert( file_.is_open() );

                try
                {
                    if ( file_.is_open() ) return file_.data();
                }
                catch ( ... )
                {
                }
          
                throw runtime_error( RetCode::IoError );
            }
        };
    }
}

#endif