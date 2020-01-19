#ifndef __JB__EXCEPTION__H__
#define __JB__EXCEPTION__H__

#include <stdexcept>
#include "ret_codes.h"

namespace jb
{
    namespace details
    {
        class exception : public std::runtime_error
        {
            RetCode error_code_ = RetCode::Ok;

        public:

            explicit exception( RetCode error_code) noexcept
                : std::runtime_error( "jb storage internal run-time error" )
                , error_code_( error_code )
            {}
            
            RetCode error_code() const noexcept { return error_code_; }
        };
    }
}

#endif
