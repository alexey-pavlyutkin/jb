






#include <gtest/gtest.h>
#include <jb/storage.h>

namespace jb
{
    namespace regression
    {
        TEST( VirtualVolume, Status )
        {
            storage<> s;

            auto [rc, virtual_volume_handle] = s.open_virtual_volume();
            ASSERT_EQ( RetCode::Ok, rc );
            ASSERT_FALSE( virtual_volume_handle.expired() );
            //
        }
    }
}