#include <gtest/gtest.h>
#include <jb/suspendable_lock.h>

namespace jb
{
    namespace regression
    {
        namespace
        {
            struct Lockable
            {
                bool locked_ = false;
                void lock() noexcept { locked_ = true; }
                void unlock() noexcept { locked_ = false; }
                bool locked() const noexcept { return locked_; }
            };
        }


        using suspendable_lock = details::suspendable_lock< Lockable >;
        using scoped_lock_suspend = typename suspendable_lock::scoped_lock_suspend;


        TEST( suspendable_lock, base )
        {
            Lockable lockable;
            EXPECT_FALSE( lockable.locked() );
            {
                suspendable_lock lock( lockable );
                EXPECT_TRUE( lockable.locked() );
                {
                    scoped_lock_suspend lock_suspend( lock );
                    EXPECT_FALSE( lockable.locked() );
                }
                EXPECT_TRUE( lockable.locked() );
            }
            EXPECT_FALSE( lockable.locked() );
        }


        TEST( suspendable_lock, move_construction )
        {
            Lockable lockable;
            EXPECT_FALSE( lockable.locked() );
            {
                suspendable_lock src( lockable );
                EXPECT_TRUE( lockable.locked() );
                {
                    suspendable_lock dst( std::move( src ) );
                    EXPECT_TRUE( lockable.locked() );
                    {
                        scoped_lock_suspend lock_suspend( dst );
                        EXPECT_FALSE( lockable.locked() );
                    }
                    EXPECT_TRUE( lockable.locked() );
                }
                EXPECT_FALSE( lockable.locked() );
            }
            EXPECT_FALSE( lockable.locked() );
        }


        TEST( suspendable_lock, move_assignment )
        {
            Lockable lockable;
            EXPECT_FALSE( lockable.locked() );
            {
                suspendable_lock dst;
                EXPECT_FALSE( lockable.locked() );
                {
                    suspendable_lock src( lockable );
                    EXPECT_TRUE( lockable.locked() );
                    //
                    dst = std::move( src );
                    EXPECT_TRUE( lockable.locked() );
                    {
                        scoped_lock_suspend lock_suspend( dst );
                        EXPECT_FALSE( lockable.locked() );
                    }
                }
                EXPECT_TRUE( lockable.locked() );
            }
            EXPECT_FALSE( lockable.locked() );
        }
    }
}