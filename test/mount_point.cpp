#include <gtest/gtest.h>
#include <jb/storage.h>

namespace jb
{
    namespace regression
    {
        struct mount_point : public ::testing::Test
        {
            using key_t = typename storage<>::key_t;
            using physical_volume_t = typename typename storage<>::physical_volume_t;
            using mount_point_t = typename storage<>::mount_point_t;
            struct private_construction : public mount_point_t
            {
                template < typename... Args >
                private_construction( Args&&... args ) : mount_point_t( std::forward< Args >( args )... ) {}
            };
        };

        TEST_F( mount_point, base )
        {
            storage<> s;
            auto [rc, physical_volume_handle] = s.open_physical_volume( "./foo.jb", 777 );
            ASSERT_EQ( RetCode::Ok, rc );
            ASSERT_FALSE( physical_volume_handle.expired() );
            //
            key_t physical_path = "/aaa/bbb/ccc";
            key_t logical_path = "/ddd/eee/fff";
            auto mp = std::make_shared< private_construction >( physical_volume_handle.lock(), physical_path, logical_path );
            ASSERT_TRUE( mp );
            //
            EXPECT_TRUE( physical_path.empty() );
            EXPECT_TRUE( logical_path.empty() );
            //
            EXPECT_EQ( physical_volume_handle.lock(), mp->physical_volume() );
            EXPECT_EQ( "/aaa/bbb/ccc", mp->physical_path() );
            EXPECT_EQ( "/ddd/eee/fff", mp->logical_path() );
            EXPECT_EQ( 777, mp->priority() );
        }
    }
}