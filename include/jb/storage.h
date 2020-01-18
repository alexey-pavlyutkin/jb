#ifndef __JB__STORAGE__H__
#define __JB__STORAGE__H__


#include "ret_codes.h"
#include <memory>
#include <mutex>
#include <unordered_set>
#include <tuple>
#include <filesystem>
#include <type_traits>


namespace jb
{
    /** Abstracts a set of compile-time settings
    */
    struct default_policy
    {
    };


    /** Implements key-value storage, takes care about volume collections

    @tparam Policy - compile-time settings
    */
    template < typename Policy = default_policy >
    class storage
    {
        /** Provides abstract volume collection

        @tparam VolumeType - volume type
        @retval reference to volume collection guard
        @retval reference to volume collection
        */
        template< typename VolumeType >
        static auto singleton()
        {
            static std::mutex guard;
            static std::unordered_set< std::shared_ptr< VolumeType > > volumes;
            return std::forward_as_tuple( guard, volumes );
        }

        /** Opens abstract volume

        @tparam VolumeType - volume type
        @tparam Args... - volume parameters
        @param args [in] - volume parameters
        @retval error code
        @retval volume handler
        @throw nothing
        */
        template< typename VolumeType, typename... Args >
        static std::tuple< RetCode, std::weak_ptr< VolumeType > > open( Args&&... args ) noexcept
        {
            try
            {
                auto [ guard, volumes ] = singleton< VolumeType >();
                auto volume = std::make_shared< VolumeType >( std::forward< Args >( args )... );
                assert( volume );

                if ( RetCode::Ok == volume->status() )
                {
                    std::unique_lock lock( guard );

                    if ( auto ok = volumes.insert( volume ).second )
                    {
                        return { RetCode::Ok, std::weak_ptr< VolumeType >( volume ) };
                    }
                    else
                    {
                        return { RetCode::UnknownError, std::weak_ptr< VolumeType >() };
                    }
                }
                else
                {
                    return { volume->status(), std::weak_ptr< VolumeType >() };
                }
            }
            catch ( const std::bad_alloc& )
            {
                return { RetCode::InsufficientMemory, std::weak_ptr< VolumeType >() };
            }
            catch ( ... )
            {
                return { RetCode::UnknownError, std::weak_ptr< VolumeType >() };
            }
        }
    
    public:

        // temporary stub
        struct virtual_volume
        {
            RetCode status() const noexcept { return RetCode::Ok; }
        };

        // temporary stub
        struct physical_volume
        {
            physical_volume( const std::filesystem::path& path, int priority ) : path_( path ), priority_( priority ) {}

            const std::filesystem::path& path() const noexcept { return path_; }
            int priority() const noexcept { return priority_; }
            RetCode status() const noexcept { return RetCode::Ok; }
            
            std::filesystem::path path_;
            int priority_ = 0;
        };


        /** Opens another virtual volume

        @retval error code
        @retval physical volume handle
        @throw nothing
        */
        [[nodiscard]]
        static std::tuple< RetCode, std::weak_ptr< virtual_volume > >
        open_virtual_volume() noexcept
        {
            return open< virtual_volume >();
        }


        /** Opens physical volume at given path with given priority

        @param [in] path - file path to physical volume
        @param [in] priority - priority to be assigned to physical volume
        @retval error code
        @retval physical volume handle
        @throw nothing
        */
        [[nodiscard]]
        static std::tuple< RetCode, std::weak_ptr< physical_volume > >
        open_physical_volume( const std::filesystem::path& path, int priority = 0 ) noexcept
        {
            return open< physical_volume >( path, priority );
        }


        /** Closes given volume

        @tparam VolumeType - volume type (virtual or physical)
        @param [in] volume - volume to be closed
        @retval error code
        @throw nothing
        */
        template< typename VolumeType >
        static RetCode close( std::weak_ptr< VolumeType > volume ) noexcept
        {
            static_assert(  std::is_same_v< VolumeType, virtual_volume > || 
                            std::is_same_v< VolumeType, physical_volume > );

            try
            {
                auto [guard, volumes] = singleton< VolumeType >();

                std::unique_lock lock( guard );

                if ( auto it = volumes.find( volume.lock() ); it != volumes.end() )
                {
                    volumes.erase( it );
                    return RetCode::Ok;
                }
                else
                {
                    return RetCode::InvalidHandle;
                }
            }
            catch ( ... )
            {
                return RetCode::UnknownError;
            }
        }

        /** Closes all open volumes, physical and virtual

        @retval error code
        @throw nothing
        */
        static RetCode close_all() noexcept
        {
            try
            {
                {
                    auto [guard, volumes] = singleton< virtual_volume >();

                    std::unique_lock lock( guard );
                    volumes.clear();
                }
                {
                    auto [guard, volumes] = singleton< physical_volume >();

                    std::unique_lock lock( guard );
                    volumes.clear();
                }

                return RetCode::Ok;
            }
            catch ( ... )
            {
                return RetCode::UnknownError;
            }
        }
    };
}

#endif
