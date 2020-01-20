#ifndef __JB__PHYSICAL_VOLUME__H__
#define __JB__PHYSICAL_VOLUME__H__


#include "ret_codes.h"
#include "exception.h"
#include <filesystem>

namespace jb
{
    namespace details
    {
        template < typename Policy >
        class physical_volume
        {
            std::filesystem::path path_;
            int priority_;

        public:

            physical_volume() = delete;
            physical_volume( physical_volume&& ) = delete;

            explicit physical_volume( const std::filesystem::path& path, int priority ) try
                : path_( std::move( std::filesystem::absolute( path ) ) )
                , priority_( priority )
            {
            }
            catch ( const std::filesystem::filesystem_error & e )
            {
                throw runtime_error( RetCode::InvalidFilePath );
            }

            const std::filesystem::path& path() const noexcept { return path_; }
            int priority() const noexcept { return priority_; }
        };
    }
}

#endif
