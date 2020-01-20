#include <gtest/gtest.h>
#include <jb/storage.h>

namespace jb
{
    namespace regression
    {
        TEST( physical_volume, base )
        {
            storage<> s;
            //
            auto [rc_1, physical_volume_handle_1 ] = s.open_physical_volume( "./foo.jb" );
            EXPECT_EQ( RetCode::Ok, rc_1 );
            EXPECT_EQ( std::filesystem::absolute( "./foo.jb" ), physical_volume_handle_1.lock()->path() );
            EXPECT_EQ( 0, physical_volume_handle_1.lock()->priority() );
            //
            auto [rc_2, physical_volume_handle_2] = s.open_physical_volume( "./boo.jb", 111 );
            EXPECT_EQ( RetCode::Ok, rc_2 );
            EXPECT_EQ( std::filesystem::absolute( "./boo.jb" ), physical_volume_handle_2.lock()->path() );
            EXPECT_EQ( 111, physical_volume_handle_2.lock()->priority() );
        }
    }
}