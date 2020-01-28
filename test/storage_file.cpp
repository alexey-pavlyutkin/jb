#include <gtest/gtest.h>
#include <jb/storage.h>
#include <jb/storage_file.h>
#include <future>


namespace jb
{
    namespace regression
    {
        struct storage_file_test : public ::testing::Test
        {
            using storage_file = jb::details::storage_file< jb::default_policies >;

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


        TEST_F( storage_file_test, exclusive_access )
        {
            {
                std::unique_ptr< storage_file > f1;
                EXPECT_NO_THROW( f1 = std::make_unique< storage_file >( "./foo.jb" ) );

                std::unique_ptr< storage_file > f2;
                EXPECT_THROW( f2 = std::make_unique< storage_file >( "./foo.jb" ), details::runtime_error );
            }

            std::unique_ptr< storage_file > f1;
            EXPECT_NO_THROW( f1 = std::make_unique< storage_file >( "./foo.jb" ) );
        }

        TEST_F( storage_file_test, exclusive_access_multithreading )
        {
            std::atomic_bool run = false;
            auto thread_number = std::thread::hardware_concurrency() - 1;

            if ( thread_number > 1 )
            {
                std::vector< std::future< std::unique_ptr< storage_file > > > futures( thread_number );

                for ( auto& f : futures )
                {
                    f = std::async( std::launch::async,
                        [&]()
                        {
                            while ( !run.load( std::memory_order_acquire ) );
                            return std::make_unique< storage_file >( "./foo.jb" );
                        }
                    );
                }

                run.store( true, std::memory_order_release );

                size_t failed_thread_number = 0;
                for ( auto& f : futures )
                {
                    try { f.get(); }
                    catch ( const details::runtime_error & e ) { ++failed_thread_number; }
                }

                EXPECT_EQ( thread_number - 1, failed_thread_number );
            }
        }

        TEST_F( storage_file_test, invalid_fpath )
        {
            std::unique_ptr< storage_file > f;
            EXPECT_THROW( f = std::make_unique< storage_file >( "./~!@#$%^&*()_+.jb" ), details::runtime_error );
        }

        TEST_F( storage_file_test, create_new_auto_grow_size )
        {
            {
                std::unique_ptr< storage_file > f;
                EXPECT_NO_THROW( f = std::make_unique< storage_file >( "./foo.jb" ) );
                EXPECT_EQ( storage_file::page_size(), f->size() );
            }
            EXPECT_EQ( storage_file::page_size(), std::filesystem::file_size( "./foo.jb" ) );
        }

        TEST_F( storage_file_test, open_existing_grow_size )
        {
            {
                std::unique_ptr< storage_file > f;
                EXPECT_NO_THROW( f = std::make_unique< storage_file >( "./foo.jb" ) );
                EXPECT_NO_THROW( f->grow() );
                EXPECT_EQ( 2 * storage_file::page_size(), f->size() );
            }
            EXPECT_EQ( 2 * storage_file::page_size(), std::filesystem::file_size( "./foo.jb" ) );
        }

        TEST_F( storage_file_test, map_invalid_offset )
        {
            std::unique_ptr< storage_file > f;
            EXPECT_NO_THROW( f = std::make_unique< storage_file >( "./foo.jb" ) );
            EXPECT_THROW( f->map( 1 ), std::logic_error );
            EXPECT_THROW( f->map( storage_file::page_size() - 1 ), std::logic_error );
            EXPECT_THROW( f->map( storage_file::page_size() / 2 ), std::logic_error );
            EXPECT_THROW( f->map( storage_file::page_size() ), std::logic_error );
        }

        TEST_F( storage_file_test, map )
        {
            {
                {
                    std::unique_ptr< storage_file > f;
                    ASSERT_NO_THROW( f = std::make_unique< storage_file >( "./foo.jb" ) );
                    ASSERT_NO_THROW( f->grow() );
                    ASSERT_NO_THROW( f->grow() );
                }

                std::unique_ptr< storage_file > f;
                ASSERT_NO_THROW( f = std::make_unique< storage_file >( "./foo.jb" ) );

                for ( auto i = 0; i < 2; ++i )
                {
                    decltype( f->map( 0 ) ) mapping_1, mapping_2;
                    EXPECT_NO_THROW( mapping_1 = f->map( i * storage_file::page_size() ) );
                    EXPECT_NO_THROW( mapping_2 = f->map( i * storage_file::page_size() ) );
                    EXPECT_TRUE( mapping_1 );
                    EXPECT_TRUE( mapping_2 );
                    //
                    char* ptr_1 = (char*)mapping_1.get();
                    char* ptr_2 = (char*)mapping_2.get();
                    ASSERT_TRUE( ptr_1 && ptr_2 );
                    //
                    for ( auto j = 0; j < storage_file::page_size(); ++j, ++ptr_1, ++ptr_2 )
                    {
                        EXPECT_EQ( *ptr_1, *ptr_2 );
                    }
                }
            }
        }
    }
}