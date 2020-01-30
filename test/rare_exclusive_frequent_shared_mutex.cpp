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
        }

        TEST_F( rare_exclusive_frequent_shared_mutex_test, try_lock_over_lock )
        {
            shared_mutex mtx;
            mtx.lock();
            auto f = std::async( std::launch::async, [&]() noexcept { return mtx.try_lock(); } );
            EXPECT_FALSE( f.get() );
        }

        TEST_F( rare_exclusive_frequent_shared_mutex_test, try_lock_shared_over_lock_shared )
        {
            shared_mutex mtx;
            mtx.lock_shared( std::hash< std::thread::id >{}( std::this_thread::get_id() ) );
            for ( size_t id = 0; id < shared_mutex::shared_lock_count(); ++id )
            {
                auto f = std::async( std::launch::async, [&]() noexcept { return mtx.try_lock_shared( id ); } );
                EXPECT_TRUE( f.get() );
            }
        }

        TEST_F( rare_exclusive_frequent_shared_mutex_test, try_lock_shared_over_lock )
        {
            shared_mutex mtx;
            mtx.lock();
            for ( size_t id = 0; id < shared_mutex::shared_lock_count(); ++id )
            {
                auto f = std::async( std::launch::async, [&]() noexcept { return mtx.try_lock_shared( id ); } );
                EXPECT_FALSE( f.get() );
            }
        }

        TEST_F( rare_exclusive_frequent_shared_mutex_test, try_lock_over_lock_shared )
        {
            shared_mutex mtx;
            for ( size_t id = 0; id < shared_mutex::shared_lock_count(); ++id )
            {
                mtx.lock_shared( id );
            }
            mtx.lock_shared( 0 );
            //
            for ( size_t id = 0; id < shared_mutex::shared_lock_count(); ++id )
            {
                auto f = std::async( std::launch::async, [&]() noexcept { return mtx.try_lock(); } );
                EXPECT_FALSE( f.get() );
                //
                mtx.unlock_shared( id );
            }
            //
            {
                auto f = std::async( std::launch::async, [&]() noexcept { return mtx.try_lock(); } );
                EXPECT_FALSE( f.get() );
            }
            //
            mtx.unlock_shared( 0 );
            //
            {
                auto f = std::async( std::launch::async, [&]() noexcept { return mtx.try_lock(); } );
                EXPECT_TRUE( f.get() );
            }
        }

        TEST_F( rare_exclusive_frequent_shared_mutex_test, unique_lock )
        {
            shared_mutex mtx;

            auto check_lock = [&]() noexcept {
                if ( mtx.try_lock_shared( 0 ) )
                {
                    mtx.unlock_shared( 0 );
                    return false;
                }
                {
                    return true;
                }
            };

            {
                unique_lock lock_1;
                EXPECT_FALSE( std::async( std::launch::async, check_lock ).get() );
                {
                    unique_lock lock_2;
                    EXPECT_FALSE( std::async( std::launch::async, check_lock ).get() );
                    {
                        unique_lock lock_3( mtx );
                        EXPECT_TRUE( std::async( std::launch::async, check_lock ).get() );
                    }
                    EXPECT_FALSE( std::async( std::launch::async, check_lock ).get() );

                    unique_lock lock_4( mtx );
                    EXPECT_TRUE( std::async( std::launch::async, check_lock ).get() );
                    {
                        unique_lock lock_5( std::move( lock_4 ) );
                        EXPECT_TRUE( std::async( std::launch::async, check_lock ).get() );
                    }
                    EXPECT_FALSE( std::async( std::launch::async, check_lock ).get() );

                    {
                        unique_lock lock_6( mtx );
                        EXPECT_TRUE( std::async( std::launch::async, check_lock ).get() );

                        lock_6.unlock();
                        EXPECT_FALSE( std::async( std::launch::async, check_lock ).get() );

                        lock_6.lock();
                        EXPECT_TRUE( std::async( std::launch::async, check_lock ).get() );

                        lock_2 = std::move( lock_6 );
                    }
                    EXPECT_TRUE( std::async( std::launch::async, check_lock ).get() );

                    lock_1.swap( lock_2 );
                }
                EXPECT_TRUE( std::async( std::launch::async, check_lock ).get() );

                lock_1 = unique_lock{};
                EXPECT_FALSE( std::async( std::launch::async, check_lock ).get() );
            }
        }

        TEST_F( rare_exclusive_frequent_shared_mutex_test, shared_lock )
        {
            shared_mutex mtx;

            auto check_lock_shared = [&]() noexcept {
                if ( mtx.try_lock() )
                {
                    mtx.unlock();
                    return false;
                }
                {
                    return true;
                }
            };

            {
                shared_lock lock_1;
                EXPECT_FALSE( std::async( std::launch::async, check_lock_shared ).get() );
                {
                    shared_lock lock_2;
                    EXPECT_FALSE( std::async( std::launch::async, check_lock_shared ).get() );
                    {
                        shared_lock lock_3( mtx, 0 );
                        EXPECT_TRUE( std::async( std::launch::async, check_lock_shared ).get() );
                    }
                    EXPECT_FALSE( std::async( std::launch::async, check_lock_shared ).get() );

                    shared_lock lock_4( mtx, 1 );
                    {
                        shared_lock lock_5( std::move( lock_4 ) );
                        EXPECT_TRUE( std::async( std::launch::async, check_lock_shared ).get() );
                    }
                    EXPECT_FALSE( std::async( std::launch::async, check_lock_shared ).get() );

                    {
                        shared_lock lock_6( mtx, 2 );
                        lock_6.unlock();
                        EXPECT_FALSE( std::async( std::launch::async, check_lock_shared ).get() );

                        lock_6.lock();
                        EXPECT_TRUE( std::async( std::launch::async, check_lock_shared ).get() );

                        lock_2 = std::move( lock_6 );
                    }
                    EXPECT_TRUE( std::async( std::launch::async, check_lock_shared ).get() );

                    lock_1.swap( lock_2 );
                }
                EXPECT_TRUE( std::async( std::launch::async, check_lock_shared ).get() );

                lock_1 = shared_lock{};
                EXPECT_FALSE( std::async( std::launch::async, check_lock_shared ).get() );
            }
        }
    }
}