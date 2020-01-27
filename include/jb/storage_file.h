#ifndef __JB__STORAGE_FILE__H__
#define __JB__STORAGE_FILE__H__


#include "ret_codes.h"
#include "exception.h"
#include <filesystem>

namespace jb
{
    namespace details
    {
        template < typename Policies >
        class storage_file : public Policies::api
        {
        public:

            storage_file() = delete;
            storage_file( storage_file&& ) = delete;

            explicit storage_file( std::filesystem::path&& path ) try
                : api( std::filesystem::absolute( path ) )
            {
                if ( !size() ) grow();
            }
            catch ( const std::filesystem::filesystem_error & e )
            {
                throw runtime_error( RetCode::InvalidFilePath, "Invalid file path" );
            }
        };
    }
}

#endif
