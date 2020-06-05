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
                    try
                    {
                        f.get();
                    }
                    catch ( const details::runtime_error & e )
                    {
                        ++failed_thread_number;
                    }
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
                ASSERT_NO_THROW( f = std::make_unique< storage_file >( "./foo.jb" ) );
                EXPECT_TRUE( f->newly_created() );
                EXPECT_EQ( storage_file::page_size(), f->size() );
            }
            EXPECT_EQ( storage_file::page_size(), std::filesystem::file_size( "./foo.jb" ) );
        }

        TEST_F( storage_file_test, open_existing_grow_size )
        {
            {
                std::unique_ptr< storage_file > f;
                ASSERT_NO_THROW( f = std::make_unique< storage_file >( "./foo.jb" ) );
            }
            {
                std::unique_ptr< storage_file > f;
                EXPECT_NO_THROW( f = std::make_unique< storage_file >( "./foo.jb" ) );
                EXPECT_FALSE( f->newly_created() );
                EXPECT_EQ( storage_file::page_size(), f->size() );
                EXPECT_NO_THROW( f->grow( 0 ) );
                EXPECT_EQ( 2 * storage_file::page_size(), f->size() );
            }
            EXPECT_EQ( 2 * storage_file::page_size(), std::filesystem::file_size( "./foo.jb" ) );
        }

        TEST_F( storage_file_test, map_invalid_offset )
        {
            std::unique_ptr< storage_file > f;
            ASSERT_NO_THROW( f = std::make_unique< storage_file >( "./foo.jb" ) );
            EXPECT_THROW( f->map_page( 1 ), std::logic_error );
            EXPECT_THROW( f->map_page( storage_file::page_size() - 1 ), std::logic_error );
            EXPECT_THROW( f->map_page( storage_file::page_size() / 2 ), std::logic_error );
            EXPECT_THROW( f->map_page( storage_file::page_size() ), std::logic_error );
        }

        TEST_F( storage_file_test, map_page )
        {
            {
                {
                    std::unique_ptr< storage_file > f;
                    ASSERT_NO_THROW( f = std::make_unique< storage_file >( "./foo.jb" ) );
                    ASSERT_NO_THROW( f->grow( 0 ) );
                    ASSERT_NO_THROW( f->grow( 0 ) );
                }

                {
                    std::unique_ptr< storage_file > f;
                    ASSERT_NO_THROW( f = std::make_unique< storage_file >( "./foo.jb" ) );

                    for ( uint64_t page_offset = 0; page_offset < 3 * storage_file::page_size(); page_offset += storage_file::page_size() )
                    {
                        decltype( f->map_page( 0 ) ) page_1, page_2;
                        EXPECT_NO_THROW( EXPECT_TRUE( page_1 = f->map_page( page_offset ) ) );
                        EXPECT_NO_THROW( EXPECT_TRUE( page_2 = f->map_page( page_offset ) ) );
                        //
                        char* ptr_1 = (char*)page_1.get();
                        char* ptr_2 = (char*)page_2.get();
                        ASSERT_TRUE( ptr_1 && ptr_2 );
                        //
                        for ( auto i = 0; i < storage_file::page_size(); ++i, ++ptr_1, ++ptr_2 )
                        {
                            *ptr_1 = static_cast< char >( i % 0x100 );
                            EXPECT_EQ( *ptr_1, *ptr_2 );
                        }
                    }
                }

                {
                    std::FILE* f = std::fopen( "./foo.jb", "rb" );
                    ASSERT_TRUE( f );

                    for ( uint64_t page_offset = 0; page_offset < 3 * storage_file::page_size(); page_offset += storage_file::page_size() )
                    {
                        std::vector< char > buffer( storage_file::page_size(), char( 0 ) );
                        ASSERT_EQ( 1, std::fread( buffer.data(), buffer.size(), 1, f ) );

                        for ( auto i = 0; i < storage_file::page_size(); ++i )
                        {
                            EXPECT_EQ( static_cast< char >( i % 0x100 ), buffer[ i ] );
                        }
                    }

                    std::fclose( f );
                }

                {
                    std::unique_ptr< storage_file > f;
                    ASSERT_NO_THROW( f = std::make_unique< storage_file >( "./foo.jb" ) );

                    for ( uint64_t page_offset = 0; page_offset < 3 * storage_file::page_size(); page_offset += storage_file::page_size() )
                    {
                        decltype( f->map_page( 0 ) ) page;
                        EXPECT_NO_THROW( EXPECT_TRUE( page = f->map_page( page_offset ) ) );
                        //
                        char* ptr = (char*)page.get();
                        ASSERT_TRUE( ptr );
                        //
                        for ( auto i = 0; i < storage_file::page_size(); ++i, ++ptr )
                        {
                            EXPECT_EQ( static_cast<char>( i % 0x100 ), *ptr );
                        }
                    }
                }
            }
        }
    }
}