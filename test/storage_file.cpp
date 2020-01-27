#include <gtest/gtest.h>
#include <jb/storage.h>
#include <jb/storage_file.h>

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
    }
}