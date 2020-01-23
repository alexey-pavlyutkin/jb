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
            using mount_point_ptr = std::shared_ptr< mount_point_t >;
            struct private_construction : public mount_point_t
            {
                template < typename... Args >
                private_construction( Args&&... args ) : mount_point_t( std::forward< Args >( args )... ) {}
            };
        };

        TEST_F( mount_point, base )
        {
            // open a physical volume
            storage<> s;
            auto [rc, physical_volume_handle] = s.open_physical_volume( "./foo.jb", 777 );
            ASSERT_EQ( RetCode::Ok, rc );
            ASSERT_FALSE( physical_volume_handle.expired() );

            // create 1st parentless mount point
            key_t physical_path = "/aaa/bbb/ccc";
            key_t logical_path = "/ddd/eee/fff";
            auto mp_1 = std::make_shared< private_construction >( 
                physical_volume_handle.lock(), 
                std::forward< key_t >( physical_path ), 
                mount_point_ptr(), 
                std::forward< key_t >( logical_path ) );
            ASSERT_TRUE( mp_1 );

            // verify move semantic
            EXPECT_TRUE( physical_path.empty() );
            EXPECT_TRUE( logical_path.empty() );

            // validate passed parameters
            EXPECT_EQ( physical_volume_handle.lock(), mp_1->physical_volume() );
            EXPECT_EQ( "/aaa/bbb/ccc", mp_1->physical_path() );
            EXPECT_EQ( mount_point_ptr(), mp_1->parent() );
            EXPECT_EQ( "/ddd/eee/fff", mp_1->logical_path() );
            EXPECT_EQ( 777, mp_1->priority() );

            // create 2nd mount pointer parented to the 1st
            auto mp_2 = std::make_shared< private_construction >(
                physical_volume_handle.lock(),
                std::forward< key_t >( physical_path ),
                mp_1,
                std::forward< key_t >( logical_path ) );
            ASSERT_TRUE( mp_2 );

            // verify parent parameter
            EXPECT_EQ( mp_1, mp_2->parent() );
        }
    }
}