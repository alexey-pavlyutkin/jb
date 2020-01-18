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
            RetCode status_ = RetCode::Ok;

        public:

            /** Default constructor

            @throw std::exception
            */
            virtual_volume() {}

            /** Deleted move constructor

            Prevents implicit copy/move construction/assignment
            */
            virtual_volume( virtual_volume&& ) = delete;

            /** Provides volume status

            @retval volume status
            @throw nothing
            */
            RetCode status() const noexcept { return status_; }
        };
    }
}

#endif
