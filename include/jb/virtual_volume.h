#ifndef __JB__VIRTUAL_VOLUME__H__
#define __JB__VIRTUAL_VOLUME__H__

#include "ret_codes.h"
#include "mount_point.h"
#include <tuple>
#include <memory>

namespace jb
{
    namespace details
    {
        template < typename Policies >
        class virtual_volume
        {
        public:

            using key_t = std::basic_string< typename Policies::key_char_t, typename Policies::key_traits_t >;
            using value_t = typename Policies::value_t;
            using mount_point_t = mount_point< Policies >;

            /** Default constructor

            @throw std::exception
            */
            virtual_volume() {}

            /** Deleted move constructor

            Prevents implicit copy/move construction/assignment
            */
            virtual_volume( virtual_volume&& ) = delete;


            std::tuple< RetCode, std::weak_ptr< mount_point_t > > mount() noexcept
            {
                return { RetCode::UnknownError, std::weak_ptr< mount_point_t > >() };
            }

            RetCode unmount( const std::weak_ptr< mount_point_t > & mp ) noexcept
            {
                return RetCode::UnknownError;
            }
        };
    }
}

#endif
