#include <gtest/gtest.h>
#include <jb/mapped_region.h>
#include <jb/suspendable_lock.h>

namespace jb
{
    namespace regression
    {
        namespace
        {
            struct SharedMutex
            {
                bool shared_lock_ = false;
                bool unique_lock_ = false;

                void lock() { unique_lock_ = true; }
                void unlock() { unique_lock_ = false; }
                void lock_shared() { shared_lock_ = true; }
                void unlock_shared() { shared_lock_ = false; }

                bool shared_lock_taken() const noexcept { return shared_lock_; }
                bool unique_lock_taken() const noexcept { return unique_lock_; }
            };
        }

        using mapped_region = details::mapped_region< SharedMutex >;
        using suspendable_lock = details::suspendable_lock< mapped_region >;
        using scoped_lock_suspend = typename suspendable_lock::scoped_lock_suspend;

        TEST( mapped_region, base )
        {
            EXPECT_FALSE( std::is_default_constructible_v< details::mapped_region< SharedMutex > > );
            EXPECT_FALSE( std::is_copy_constructible_v< details::mapped_region< SharedMutex > > );
            EXPECT_FALSE( std::is_copy_assignable_v< details::mapped_region< SharedMutex > > );
            EXPECT_FALSE( std::is_move_constructible_v< details::mapped_region< SharedMutex > > );
            EXPECT_FALSE( std::is_move_assignable_v< details::mapped_region< SharedMutex > > );
            //
            SharedMutex sm;
            std::string path = "./foo.jb";
            mapped_region mr( sm, path, 0 );
            EXPECT_FALSE( sm.shared_lock_taken() );
            EXPECT_FALSE( sm.unique_lock_taken() );
            EXPECT_EQ( path, mr.path() );
            EXPECT_EQ( 0, mr.offset() );
            EXPECT_EQ( boost::iostreams::mapped_file::alignment(), mr.size() );
            EXPECT_FALSE( mr.is_open() );
#ifdef _DEBUG
            EXPECT_DEATH( mr.data(), "is_open" );
#else
            EXPECT_THROW( mr.data(), details::runtime_error );
#endif
            //
            {
                suspendable_lock lock;
#ifdef _DEBUG
                EXPECT_DEATH( lock = suspendable_lock( mr ), "false" );
#else
                EXPECT_NO_THROW( lock = suspendable_lock( mr ) );
#endif
                EXPECT_FALSE( sm.shared_lock_taken() );
                EXPECT_FALSE( sm.unique_lock_taken() );
                EXPECT_FALSE( mr.is_open() );
            }
        }
    }
}