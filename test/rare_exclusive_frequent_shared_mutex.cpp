#include <gtest/gtest.h>
#include <jb/rare_exclusive_frequent_shared_mutex.h>
#include <future>

namespace jb
{
    namespace regression
    {
        struct rare_exclusive_frequent_shared_mutex_test : public ::testing::Test
        {
            using shared_mutex = details::rare_exclusive_frequent_shared_mutex<>;
            using unique_lock = typename shared_mutex::unique_lock;
            using shared_lock = typename shared_mutex::shared_lock;
        };

        TEST_F( rare_exclusive_frequent_shared_mutex_test, try_lock )
        {
            shared_mutex mtx;
            EXPECT_TRUE( mtx.try_lock() );
        }

        TEST_F( rare_exclusive_frequent_shared_mutex_test, try_lock_shared )
        {
            shared_mutex mtx;
            for ( size_t id = 0; id < shared_mutex::shared_lock_count(); ++id )
            {
                EXPECT_TRUE( mtx.try_lock_shared( id ) );
            }
            EXPECT_TRUE( mtx.try_lock_shared( 0 ) );
        }

        TEST_F( rare_exclusive_frequent_shared_mutex_test, try_lock_shared_over_unique )
        {
            shared_mutex mtx;
            for ( size_t id = 0; id < shared_mutex::shared_lock_count(); ++id )
            {
                ASSERT_TRUE( mtx.try_lock() );
                EXPECT_FALSE( mtx.try_lock_shared( id ) );
                mtx.unlock();
                EXPECT_TRUE( mtx.try_lock_shared( id ) );
                mtx.unlock_shared( id );
            }
        }

        TEST_F( rare_exclusive_frequent_shared_mutex_test, try_lock_unique_over_shared )
        {
            shared_mutex mtx;
            for ( size_t id = 0; id < shared_mutex::shared_lock_count(); ++id )
            {
                ASSERT_TRUE( mtx.try_lock_shared( id ) );
            }
            ASSERT_TRUE( mtx.try_lock_shared( 0 ) );
            //
            for ( size_t id = 0; id < shared_mutex::shared_lock_count(); ++id )
            {
                EXPECT_FALSE( mtx.try_lock() );
                mtx.unlock_shared( id );
            }
            EXPECT_FALSE( mtx.try_lock() );
            mtx.unlock_shared( 0 );
            EXPECT_TRUE( mtx.try_lock() );
        }

    }
}