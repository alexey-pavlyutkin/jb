#ifndef __JB__EXCEPTION__H__
#define __JB__EXCEPTION__H__

#include <stdexcept>
#include "ret_codes.h"

namespace jb
{
    namespace details
    {
        class runtime_error : public std::runtime_error
        {
            RetCode error_code_ = RetCode::Ok;

        public:

            explicit runtime_error( RetCode error_code, const char * what = nullptr ) noexcept
                : std::runtime_error( what ? what : "internal run-time error detected" )
                , error_code_( error_code )
            {}
            
            RetCode error_code() const noexcept { return error_code_; }
        };
    }
}

#endif
