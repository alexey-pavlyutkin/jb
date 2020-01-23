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
        TEST( suspendable_lock, base )
        {
            Lockable lockable;
            EXPECT_FALSE( lockable.locked() );
            {
                details::suspendable_lock< Lockable > lock( lockable );
                EXPECT_TRUE( lockable.locked() );
                {
                    details::suspendable_lock< Lockable >::scoped_lock_suspend lock_suspend( lock );
                    EXPECT_FALSE( lockable.locked() );
                }
                EXPECT_TRUE( lockable.locked() );
            }
            EXPECT_FALSE( lockable.locked() );
        }

        TEST( suspendable_lock, move_constuction )
        {
            Lockable lockable;
            EXPECT_FALSE( lockable.locked() );
            {
                details::suspendable_lock< Lockable > src( lockable );
                EXPECT_TRUE( lockable.locked() );
                {
                    details::suspendable_lock< Lockable > dst( std::move( src ) );
                    EXPECT_TRUE( lockable.locked() );
                    {
                        details::suspendable_lock< Lockable >::scoped_lock_suspend lock_suspend( dst );
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
                details::suspendable_lock< Lockable > dst;
                EXPECT_FALSE( lockable.locked() );
                {
                    details::suspendable_lock< Lockable > src( lockable );
                    EXPECT_TRUE( lockable.locked() );
                    //
                    dst = std::move( src );
                    EXPECT_TRUE( lockable.locked() );
                    {
                        details::suspendable_lock< Lockable >::scoped_lock_suspend lock_suspend( dst );
                        EXPECT_FALSE( lockable.locked() );
                    }
                }
                EXPECT_TRUE( lockable.locked() );
            }
            EXPECT_FALSE( lockable.locked() );
        }
    }
}