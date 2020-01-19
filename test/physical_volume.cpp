#include <gtest/gtest.h>
#include <jb/storage.h>

namespace jb
{
    namespace regression
    {
        TEST( PhysicalVolume, FilePath )
        {
            storage<> s;
            //
            auto [rc, physical_volume_handle] = s.open_physical_volume( "./foo.jb" );
            EXPECT_EQ( RetCode::Ok, rc );
            EXPECT_EQ( std::filesystem::absolute( "./foo.jb" ), physical_volume_handle.lock()->path() );
        }

        TEST( PhysicalVolume, Priority )
        {
            storage<> s;
            //
            auto [rc, physical_volume_handle] = s.open_physical_volume( "./foo.jb", 111 );
            EXPECT_EQ( RetCode::Ok, rc );
            EXPECT_EQ( 111, physical_volume_handle.lock()->priority() );
        }
    }
}