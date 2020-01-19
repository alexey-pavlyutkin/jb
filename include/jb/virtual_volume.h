#ifndef __JB__VIRTUAL_VOLUME__H__
#define __JB__VIRTUAL_VOLUME__H__

#include "ret_codes.h"

namespace jb
{
    namespace details
    {
        template < typename Policies >
        class virtual_volume
        {
        public:

            /** Default constructor

            @throw std::exception
            */
            virtual_volume() {}

            /** Deleted move constructor

            Prevents implicit copy/move construction/assignment
            */
            virtual_volume( virtual_volume&& ) = delete;
        };
    }
}

#endif
