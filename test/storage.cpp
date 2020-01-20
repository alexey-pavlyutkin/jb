#include <gtest/gtest.h>
#include <jb/storage.h>


namespace jb {

    namespace regression {

        TEST( storage, virtual_volume_open_close )
        {
            storage<> s;

            // open a virtual volume
            auto [rc, virtual_volume_handle] = s.open_virtual_volume();
            EXPECT_EQ( RetCode::Ok, rc );
            EXPECT_FALSE( virtual_volume_handle.expired() );

            // lock the volume for using
            auto virtual_volume = virtual_volume_handle.lock();

            // close volume
            EXPECT_EQ( RetCode::Ok, s.close( virtual_volume_handle ) );
            EXPECT_EQ( RetCode::InvalidHandle, s.close( virtual_volume_handle ) );

            // check that in-use volume still alive
            EXPECT_FALSE( virtual_volume_handle.expired() );

            // release volume and check that it dies
            virtual_volume.reset();
            EXPECT_TRUE( virtual_volume_handle.expired() );
        }

        TEST( storage, physical_volume_open_close )
        {
            storage<> s;

            // open physical volume and check creation parameters
            auto [rc1, physical_volume_handle_1] = s.open_physical_volume( "./foo.jb" );
            EXPECT_EQ( RetCode::Ok, rc1 );
            EXPECT_FALSE( physical_volume_handle_1.expired() );
            
            // open another physical volume and check parameters
            auto [rc2, physical_volume_handle_2] = s.open_physical_volume( "./boo.jb", 1 );
            EXPECT_EQ( RetCode::Ok, rc2 );
            EXPECT_FALSE( physical_volume_handle_2.expired() );

            // close 1st volume
            EXPECT_EQ( RetCode::Ok, s.close( physical_volume_handle_1 ) );
            EXPECT_EQ( RetCode::InvalidHandle, s.close( physical_volume_handle_1 ) );
            EXPECT_TRUE( physical_volume_handle_1.expired() );
            EXPECT_FALSE( physical_volume_handle_2.expired() );

            // try to close 2nd volume while it stays in use
            auto physical_volume_2 = physical_volume_handle_2.lock();
            EXPECT_EQ( RetCode::Ok, s.close( physical_volume_handle_2 ) );
            EXPECT_EQ( RetCode::InvalidHandle, s.close( physical_volume_handle_2 ) );
            EXPECT_FALSE( physical_volume_handle_2.expired() );
            physical_volume_2.reset();
            EXPECT_TRUE( physical_volume_handle_2.expired() );
        }

        TEST( storage, close_all )
        {
            storage<> s;

            auto [rc1, virtual_volume_handle] = s.open_virtual_volume();
            EXPECT_EQ( RetCode::Ok, rc1 );
            EXPECT_FALSE( virtual_volume_handle.expired() );

            // open physical volume and check creation parameters
            auto [rc2, physical_volume_handle_1] = s.open_physical_volume( "./foo.jb" );
            EXPECT_EQ( RetCode::Ok, rc2 );
            EXPECT_FALSE( physical_volume_handle_1.expired() );

            // open another physical volume and check parameters
            auto [rc3, physical_volume_handle_2] = s.open_physical_volume( "./boo.jb", 1 );
            EXPECT_EQ( RetCode::Ok, rc3 );
            EXPECT_FALSE( physical_volume_handle_2.expired() );

            // get 1st volume in use
            auto physical_volume_1 = physical_volume_handle_1.lock();

            // close all handles
            EXPECT_EQ( RetCode::Ok, s.close_all() );
            EXPECT_FALSE( physical_volume_handle_1.expired() );

            // release the volume
            physical_volume_1.reset();
            EXPECT_TRUE( physical_volume_handle_1.expired() );
        }
    }
}
