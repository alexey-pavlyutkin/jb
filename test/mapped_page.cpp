#include <gtest/gtest.h>
#include <jb/mapped_page.h>
#include <jb/scoped_lock_suspend.h>
#include <mutex>

namespace jb
{
    namespace regression
    {
        struct mapped_page_test : public ::testing::Test
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

            using mapped_page = details::mapped_page< SharedMutex >;
            using suspendable_lock = std::unique_lock< mapped_page >;
            using lock_suspend = details::scoped_lock_suspend< mapped_page >;

            void deploy( const std::filesystem::path& fname, size_t fsize )
            {
                auto f = std::fopen( fname.string().c_str(), "w" );
                ASSERT_TRUE( f );
                std::fclose( f );
                ASSERT_NO_THROW( std::filesystem::resize_file( fname, fsize ) );
            }

            virtual void TearDown() override
            {
                for ( auto& file : std::filesystem::directory_iterator( "." ) )
                {
                    auto path = file.path();
                    if ( path.extension() == ".jb" )
                    {
                        std::filesystem::remove( file.path() );
                    }
                }
            }
        };

        TEST_F( mapped_page_test, file_not_exist )
        {
            SharedMutex resize_murex;
            std::string path = "./foo.jb";
            //
            mapped_page page( resize_murex, path, 0 );
            //
            EXPECT_EQ( path, page.path() );
            EXPECT_EQ( 0, page.offset() );
            EXPECT_EQ( boost::iostreams::mapped_file::alignment(), page.size() );
            //
            EXPECT_FALSE( page.is_open() );
            EXPECT_FALSE( resize_murex.shared_lock_taken() );
            EXPECT_FALSE( resize_murex.unique_lock_taken() );
            //
            EXPECT_THROW( page.lock(), details::runtime_error );
            EXPECT_FALSE( page.is_open() );
            EXPECT_FALSE( resize_murex.shared_lock_taken() );
            EXPECT_FALSE( resize_murex.unique_lock_taken() );
            //
            EXPECT_THROW( page.data(), details::runtime_error );
        }

        TEST_F( mapped_page_test, file_zero_size )
        {
            SharedMutex resize_murex;
            std::string path = "./foo.jb";
            //
            deploy( path, 0 );
            //
            mapped_page page( resize_murex, path, 0 );
            //
            EXPECT_FALSE( page.is_open() );
            EXPECT_FALSE( resize_murex.shared_lock_taken() );
            EXPECT_FALSE( resize_murex.unique_lock_taken() );
            //
            EXPECT_THROW( page.lock(), details::runtime_error );
            EXPECT_FALSE( page.is_open() );
            EXPECT_FALSE( resize_murex.shared_lock_taken() );
            EXPECT_FALSE( resize_murex.unique_lock_taken() );
            //
            EXPECT_THROW( page.data(), details::runtime_error );
        }

        TEST_F( mapped_page_test, offset_out_of_range )
        {
            SharedMutex resize_murex;
            std::string path = "./foo.jb";
            //
            deploy( path, mapped_page::size() );
            //
            mapped_page page( resize_murex, path, mapped_page::size() );
            //
            EXPECT_FALSE( page.is_open() );
            EXPECT_FALSE( resize_murex.shared_lock_taken() );
            EXPECT_FALSE( resize_murex.unique_lock_taken() );
            //
            EXPECT_THROW( page.lock(), details::runtime_error );
            EXPECT_FALSE( page.is_open() );
            EXPECT_FALSE( resize_murex.shared_lock_taken() );
            EXPECT_FALSE( resize_murex.unique_lock_taken() );
            //
            EXPECT_THROW( page.data(), details::runtime_error );
        }


        TEST_F( mapped_page_test, base )
        {
            SharedMutex resize_murex;
            std::string path = "./foo.jb";
            //
            deploy( path, mapped_page::size() );
            //
            mapped_page page( resize_murex, path, mapped_page::size() );
            //
            EXPECT_FALSE( page.is_open() );
            EXPECT_FALSE( resize_murex.shared_lock_taken() );
            EXPECT_FALSE( resize_murex.unique_lock_taken() );
            //
            EXPECT_THROW( page.lock(), details::runtime_error );
            EXPECT_FALSE( page.is_open() );
            EXPECT_FALSE( resize_murex.shared_lock_taken() );
            EXPECT_FALSE( resize_murex.unique_lock_taken() );
            //
            EXPECT_THROW( page.data(), details::runtime_error );
        }
    }
}