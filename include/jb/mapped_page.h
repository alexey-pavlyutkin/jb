#ifndef __JB__MAPPED_PAGE__H__
#define __JB__MAPPED_PAGE__H__


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
        class mapped_page
        {
            SharedMutex& resize_guard_;
            std::shared_lock< SharedMutex > resize_lock_;
            const std::string & path_;
            const size_t offset_;
            boost::iostreams::mapped_file file_;
            alignas( std::hardware_destructive_interference_size ) std::atomic_int lock_count_;

        public:

            mapped_page() = delete;
            mapped_page( mapped_page&& ) = delete;

            mapped_page( SharedMutex & resize_guard, const std::string& path, size_t offset ) try
                : resize_guard_( resize_guard )
                , path_( path )
                , offset_( offset )
                , lock_count_( 0 )
            {
                assert( offset_ % size() == 0 && "Invalid offset" );
            }
            catch ( ... )
            {
                throw runtime_error( RetCode::IoError, "Unable to create mapped page" );
            }

            ~mapped_page()
            {
            }

            const std::string & path() const noexcept { return path_; }
            size_t offset() const noexcept { return offset_; }

            static size_t size()
            {
                try
                {
                    return static_cast<size_t>( boost::iostreams::mapped_file::alignment() );
                }
                catch ( ... )
                {
                    throw runtime_error( RetCode::IoError, "An error occured upon determining memory granularity" );
                }
            }
            
            bool is_open() const
            {
                try
                {
                    return file_.is_open();
                }
                catch ( ... )
                {
                    throw runtime_error( RetCode::IoError, "An error occured upon accessing mapped file" );
                }
            }

            void lock()
            {
                auto lock_count = lock_count_.fetch_add( 1, std::memory_order_acq_rel );

                if ( !lock_count ) try
                {
                    // take shared lock over the file to diable resizing
                    std::shared_lock< SharedMutex > resize_lock( resize_guard_ );

                    // open the file
                    file_.open( path_, boost::iostreams::mapped_file_base::readwrite, size(), offset_ );

                    // preserve the lock
                    resize_lock_ = std::move( resize_lock );
                }
                catch ( ... )
                {
                    throw runtime_error( RetCode::IoError, "An error occured upon opening mapped file" );
                }
            }


            /** Decrease lock count, and if there is no locks anymore
            */
            void unlock()
            {
                auto lock_count = lock_count_.fetch_sub( 1, std::memory_order_acq_rel );
                assert( !lock_count );
                
                if ( 1 == lock_count ) try
                {
                    // close the file
                    file_.close();

                    // release shared lock and enable resizing
                    std::shared_lock< SharedMutex > resize_lock( std::move( resize_lock_ ) );
                }
                catch ( ... )
                {
                    throw runtime_error( RetCode::IoError, "An error occured upon closing mapped file" );
                }
            }

            /** Provides access to mapped region memory space

            @retval
            @throw runtime_error if there is an error
            */
            char* data() const
            {
                try
                {
                    auto p = file_.data();
                    if ( p ) return p;
                }
                catch ( ... )
                {
                }

                throw runtime_error( RetCode::IoError, "An error occured upon accessing mapped file" );
            }
        };
    }
}

#endif