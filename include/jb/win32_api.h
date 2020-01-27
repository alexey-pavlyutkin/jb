#ifndef __JB__WIN32_API__H__
#define __JB__WIN32_API__H__


#include "ret_codes.h"
#include "exception.h"
#include <memory>
#include <filesystem>
#include <atomic>
#include <new>
#include <array>
#include <cstdio>
#include <exception>
#include <thread>
#include <windows.h>


namespace jb
{
    namespace win32
    {
        class api
        {
            struct close_handle
            {
                void operator()( HANDLE handle )
                {
                    if ( INVALID_HANDLE_VALUE != handle ) ::CloseHandle( handle );
                }
            };
            using safe_handle = std::unique_ptr < void, close_handle >;

            alignas( std::hardware_destructive_interference_size ) std::atomic_size_t map_count_ = 0;
            alignas( std::hardware_destructive_interference_size ) std::atomic_bool mapping_exists_ = false;
            std::filesystem::path path_;
            safe_handle exclusive_lock_;
            safe_handle file_;
            safe_handle mapping_;

        public:
            
            api() = delete;
            api( api&& ) = delete;

            explicit api( std::filesystem::path&& path ) : path_( std::move( path ) )
            {
                // make-up unique mutex name of stack
                std::array< char, 20 > mutex_name;
                snprintf( mutex_name.data(), 20, "jb_%zx", std::filesystem::hash_value( path_ ) );

                // try to create named mutex
                exclusive_lock_ = safe_handle( ::CreateMutexA( NULL, FALSE, mutex_name.data() ), close_handle() );
                if ( !exclusive_lock_ )
                {
                    // notify that unknown error occuder
                    throw details::runtime_error( RetCode::UnknownError, "Unable to create named mutex" );
                }
                else if ( ERROR_ALREADY_EXISTS == ::GetLastError() )
                {
                    // if mutex already exists -> notify that file is already in use
                    throw details::runtime_error( RetCode::AlreadyInUse, "The file is already in use" );
                }

                // try to open or create the file
                file_ = safe_handle( ::CreateFileW( path_.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ), close_handle() );
                if ( INVALID_HANDLE_VALUE == file_.get() )
                {
                    throw details::runtime_error( RetCode::CannotOpenFile, "Unable to open/create storage file" );
                }
            }

            static size_t page_size() noexcept
            {
                SYSTEM_INFO info;
                ::GetSystemInfo( &info );
                return static_cast< size_t >( info.dwAllocationGranularity );
            }

            size_t size() const
            {
                if ( INVALID_HANDLE_VALUE == file_.get() )
                {
                    throw std::logic_error( "An attempt to get size of non-existing file" );
                }

                LARGE_INTEGER sz;
                if ( !::GetFileSizeEx( file_.get(), &sz ) )
                {
                    throw details::runtime_error( RetCode::IoError, "Unable to get size of storage file" );
                }

                return static_cast<size_t>( sz.QuadPart );
            }

            void grow()
            {
                assert( INVALID_HANDLE_VALUE != file_.get() );

                if ( mapping_exists_.load( std::memory_order_acquire ) )
                {
                    throw std::logic_error( "grow() called for unmapped file" );
                }

                LARGE_INTEGER inc;
                inc.QuadPart = static_cast< LONGLONG >( page_size() );
                if ( !::SetFilePointerEx( file_.get(), inc, NULL, FILE_END ) || !::SetEndOfFile( file_.get() ) )
                {
                    throw details::runtime_error( RetCode::IoError, "Unable to resize file" );
                }
            }

            void * map( size_t offset )
            {
                assert( INVALID_HANDLE_VALUE != file_.get() );

                // check offset
                if ( offset % page_size() )
                {
                    throw std::logic_error( "Requested mapping offset conflicts with memory granuarity" );
                }
                else if ( offset >= size() )
                {
                    throw std::logic_error( "Requested mapping offset exceeds file size" );
                }

                if ( map_count_.fetch_add( 1, std::memory_order_acq_rel ) )
                {
                    // create mapping object
                    safe_handle mapping( ::CreateFileMappingW( file_.get(), NULL , PAGE_READWRITE , 0, 0, NULL ), close_handle() );
                    if ( INVALID_HANDLE_VALUE == mapping.get() )
                    {
                        throw details::runtime_error( RetCode::IoError, "Unable to create mapping object" );
                    }

                    // preserve mapping object and let other thread know that mapping object exists
                    mapping_ = std::move( mapping );
                    mapping_exists_.store( true, std::memory_order_release );
                }

                // wait for mapping object 
                while ( !mapping_exists_.load( std::memory_order_acquire ) ) std::this_thread::yield();

                //
                static constexpr auto dword_delimiter = ( 1ULL << 32 );
            }

            void unmap( void * base_address )
            {
                if ( !::UnmapViewOfFile( base_address ) )
                {
                    throw details::runtime_error( RetCode::IoError, "An error occured upon unmapping" );
                }
            }
        };
    }
}

#endif
