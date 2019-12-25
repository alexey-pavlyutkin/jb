#ifndef __JB__STORAGE__H__
#define __JB__STORAGE__H__


#include "ret_codes.h"
#include <memory>


namespace jb
{
    struct default_policy
    {

    };

    template < typename Policy = default_policy >
    class storage
    {
    public:

        struct virtual_volume;
        struct physical_volume;

        std::tuple< RetCode, std::weak_ptr< virtual_volume > >
        static open_virtual_volume() noexcept
        {
            return open< virtual_volume >();
        }
    };
}

#endif
