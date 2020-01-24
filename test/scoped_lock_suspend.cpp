#include <gtest/gtest.h>
#include <jb/scoped_lock_suspend.h>

namespace jb
{
    namespace regression
    {
        struct scoped_lock_suspend_test : public ::testing::Test
        {
            struct mutex
            {
                bool locked_ = false;
                void lock() noexcept { locked_ = true; }
                void unlock() noexcept { locked_ = false; }
                bool locked() const noexcept { return locked_; }
            };

            using suspendable_lock = std::unique_lock< mutex >;
            using scoped_lock_suspend = details::scoped_lock_suspend< mutex >;
        };


        TEST_F( scoped_lock_suspend_test, positive )
        {
            mutex mtx;
            EXPECT_FALSE( mtx.locked() );
            {
                suspendable_lock lock( mtx );
                EXPECT_TRUE( mtx.locked() );
                {
                    scoped_lock_suspend lock_suspend( lock );
                    EXPECT_FALSE( mtx.locked() );
                }
                EXPECT_TRUE( mtx.locked() );
            }
            EXPECT_FALSE( mtx.locked() );
        }


        TEST_F( scoped_lock_suspend_test, except_on_dummy_lock )
        {
            suspendable_lock lock;
            {
#ifdef _DEBUG
                EXPECT_DEATH( scoped_lock_suspend lock_suspend( lock ), "Assertion failed" );
#else
                EXPECT_THROW( scoped_lock_suspend lock_suspend( lock ), details::runtime_error );
#endif
            }
        }


        TEST_F( scoped_lock_suspend_test, except_on_untaken_lock )
        {
            mutex mtx;
            EXPECT_FALSE( mtx.locked() );
            {
                suspendable_lock lock( mtx );
                EXPECT_TRUE( mtx.locked() );
                {
                    lock.unlock();
                    EXPECT_FALSE( mtx.locked() );

#ifdef _DEBUG
                    EXPECT_DEATH( scoped_lock_suspend lock_suspend( lock ), "Assertion failed" );
#else
                    EXPECT_THROW( scoped_lock_suspend lock_suspend( lock ), details::runtime_error );
#endif
                }
            }
        }


        TEST_F( scoped_lock_suspend_test, except_on_moved_lock )
        {
            mutex mtx;
            EXPECT_FALSE( mtx.locked() );
            {
                suspendable_lock lock( mtx );
                EXPECT_TRUE( mtx.locked() );
#ifdef _DEBUG
                EXPECT_DEATH(
                    {
                        scoped_lock_suspend lock_suspend( lock );
                        EXPECT_FALSE( mtx.locked() );
                        suspendable_lock another( std::move( lock ) );
                    },
                    "Assertion failed"
                );
#else
                EXPECT_THROW(
                    {
                        scoped_lock_suspend lock_suspend( lock );
                        EXPECT_FALSE( mtx.locked() );
                        suspendable_lock another( std::move( lock ) );
                    }, 
                    details::runtime_error
                );
#endif
            }
        }
    }
}