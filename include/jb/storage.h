#ifndef __JB__STORAGE__H__
#define __JB__STORAGE__H__


#include "ret_codes.h"
#include "virtual_volume.h"
#include "physical_volume.h"
#include "mount_point.h"
#include "exception.h"
#include <memory>
#include <mutex>
#include <unordered_set>
#include <tuple>
#include <filesystem>
#include <type_traits>
#include <variant>
#include <string>


namespace jb
{
    /** Abstracts a set of compile-time settings
    */
    struct default_policies
    {
        using key_char_t = char;
        using key_traits_t = std::char_traits< key_char_t >;
        using value_t = std::variant< int, double, std::string, std::wstring >;
    };


    /** Implements key-value storage, takes care about volume collections

    @tparam Policies - compile-time settings
    */
    template < typename Policies = default_policies >
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


        /** Lets call std::make_shared<> for classes with private constructors
        */
        template < typename T >
        struct private_construction : public T
        {
            template < typename... Args >
            private_construction( Args&&... args ) : T( std::forward< Args >( args )... ) {}
        };


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
                auto volume = std::make_shared< private_construction< VolumeType > >( std::forward< Args >( args )... );

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
            catch ( const details::runtime_error & e )
            {
                return { e.error_code(), std::weak_ptr< VolumeType >() };
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

        using key_t = std::basic_string< typename Policies::key_char_t, typename Policies::key_traits_t >;
        using value_t = typename Policies::value_t;
        using virtual_volume_t = details::virtual_volume< Policies >;
        using physical_volume_t = details::physical_volume< Policies >;
        using mount_point_t = details::mount_point< Policies >;


        /** Opens another virtual volume

        @retval error code
        @retval physical volume handle
        @throw nothing
        */
        [[nodiscard]]
        static std::tuple< RetCode, std::weak_ptr< virtual_volume_t > >
        open_virtual_volume() noexcept
        {
            return open< virtual_volume_t >();
        }


        /** Opens physical volume at given path with given priority

        @param [in] path - file path to physical volume
        @param [in] priority - priority to be assigned to physical volume
        @retval error code
        @retval physical volume handle
        @throw nothing
        */
        [[nodiscard]]
        static std::tuple< RetCode, std::weak_ptr< physical_volume_t > >
        open_physical_volume( const std::filesystem::path& path, int priority = 0 ) noexcept
        {
            return open< physical_volume_t >( path, priority );
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
            static_assert(  std::is_same_v< VolumeType, virtual_volume_t > || 
                            std::is_same_v< VolumeType, physical_volume_t > );

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
                    auto [guard, volumes] = singleton< virtual_volume_t >();

                    std::unique_lock lock( guard );
                    volumes.clear();
                }
                {
                    auto [guard, volumes] = singleton< physical_volume_t >();

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
