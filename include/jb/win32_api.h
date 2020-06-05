#ifndef __JB__WIN32_API__H__
#define __JB__WIN32_API__H__


#include "ret_codes.h"
#include "exception.h"
#include "rare_exclusive_frequent_shared_mutex.h"
#include <filesystem>
#include <cstdio>
#include <exception>
#include <windows.h>


namespace jb
{
    namespace win32
    {
        class api
        {
            struct close_handle
            {
                void operator()( HANDLE handle ) noexcept
                {
                    if ( INVALID_HANDLE_VALUE != handle ) ::CloseHandle( handle );
                }
            };

            struct unmap_area
            {
                void operator()( void* p ) noexcept { ::UnmapViewOfFile( p ); }
            };

            using safe_handle = std::unique_ptr < void, close_handle >;

            static size_t get_page_size() noexcept
            {
                SYSTEM_INFO info;
                ::GetSystemInfo( &info );
                return static_cast<size_t>( info.dwAllocationGranularity );
            }

            std::filesystem::path path_;
            bool newly_created_ = false;
            inline static const size_t page_size_ = get_page_size();
            safe_handle interprocess_lock_;
            safe_handle file_;
            safe_handle mapping_;
            details::rare_exclusive_frequent_shared_mutex<> resize_guard_;
            size_t size_;

            safe_handle get_interprocess_lock()
            {
                // make-up unique mutex name of stack
                std::array< char, 20 > mutex_name;
                snprintf( mutex_name.data(), 20, "jb_%zx", std::filesystem::hash_value( path_ ) );

                // try to create named mutex
                safe_handle lock( ::CreateMutexA( NULL, FALSE, mutex_name.data() ), close_handle() );
                if ( !lock )
                {
                    // notify that unknown error occuder
                    throw details::runtime_error( RetCode::UnknownError, "Unable to create named mutex" );
                }
                else if ( ERROR_ALREADY_EXISTS == ::GetLastError() )
                {
                    // if mutex already exists -> notify that file is already in use
                    throw details::runtime_error( RetCode::AlreadyInUse, "The file is already in use" );
                }

                return lock;
            }

            safe_handle open_file()
            {
                safe_handle file( ::CreateFileW( path_.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ), close_handle() );
                if ( INVALID_HANDLE_VALUE == file_.get() )
                {
                    throw details::runtime_error( RetCode::CannotOpenFile, "Unable to open/create storage file" );
                }

                if ( ERROR_ALREADY_EXISTS != ::GetLastError() )
                {
                    newly_created_ = true;

                    LARGE_INTEGER inc;
                    inc.QuadPart = static_cast<LONGLONG>( page_size() );
                    if ( !::SetFilePointerEx( file.get(), inc, NULL, FILE_END ) || !::SetEndOfFile( file.get() ) )
                    {
                        throw details::runtime_error( RetCode::IoError, "Unable to resize file" );
                    }
                }

                return file;
            }

            safe_handle create_mapping()
            {
                assert( INVALID_HANDLE_VALUE != file_.get() );

                // create mapping object
                safe_handle mapping{ ::CreateFileMappingW( file_.get(), NULL, PAGE_READWRITE, 0, 0, NULL ), close_handle() };
                if ( !mapping )
                {
                    throw details::runtime_error( RetCode::IoError, "Unable to create mapping object" );
                }

                return mapping;
            }

        public:
            
            api() = delete;
            api( api&& ) = delete;

            explicit api( std::filesystem::path&& path )
                : path_( std::move( path ) )
                , interprocess_lock_( get_interprocess_lock() )
                , file_( open_file() )
                , mapping_( create_mapping() )
            {
            }

            static size_t page_size() noexcept
            {
                return page_size_;
            }

            bool newly_created() const noexcept
            {
                return newly_created_;
            }

            size_t size() const
            {
                LARGE_INTEGER sz;
                if ( !::GetFileSizeEx( file_.get(), &sz ) )
                {
                    throw details::runtime_error( RetCode::IoError, "Unable to get size of storage file" );
                }

                return static_cast<size_t>( sz.QuadPart );
            }

            bool grow( size_t current_size )
            {
                assert( INVALID_HANDLE_VALUE != file_.get() );

                details::rare_exclusive_frequent_shared_mutex<>::unique_lock lock( resize_guard_ );
                {
                    if ( size_ == current_size )
                    {
                        mapping_ = std::move( safe_handle{} );

                        LARGE_INTEGER inc;
                        inc.QuadPart = static_cast<LONGLONG>( page_size() );
                        if ( !::SetFilePointerEx( file_.get(), inc, NULL, FILE_END ) || !::SetEndOfFile( file_.get() ) )
                        {
                            throw details::runtime_error( RetCode::IoError, "Unable to resize file" );
                        }

                        size_ += page_size();
                        mapping_ = std::move( create_mapping() );

                        return true;
                    }
                }

                return false;
            }

            using safe_mapped_area = std::unique_ptr< void, unmap_area >;

            safe_mapped_area map_page( size_t offset )
            {
                assert( mapping_ );

                // check offset
                if ( offset % page_size() )
                {
                    throw std::logic_error( "Requested mapping offset conflicts with memory granuarity" );
                }
                else if ( offset + page_size() >= size() )
                {
                    throw std::logic_error( "Requested mapping offset exceeds file size" );
                }

                safe_mapped_area mapped_page{
                    ::MapViewOfFile( mapping_.get(), FILE_MAP_WRITE, offset / ( 1ULL << 32 ), offset % ( 1ULL << 32 ), page_size() ),
                    unmap_area{} 
                };

                if ( !mapped_page )
                {
                    throw details::runtime_error( RetCode::IoError, "Unable to map file into memory" );
                }

                return mapped_page;
            }
        };
    }
}

#endif
