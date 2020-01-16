cmake_minimum_required( VERSION 3.13 )

#
# try find GTest as installed target
#
find_package( GTest )

#
# if GTest is not installed
#
if ( NOT GTEST_FOUND )

    #
    # define path for GTest as external project
    #
    set( external_dir ${PROJECT_SOURCE_DIR}/externals )
    set( gtest_dir ${external_dir}/googletest )

    #
    # try to find GTest sources in externals
    #
    find_path( GTEST_CMAKE CMakeLists.txt HINTS ${gtest_dir} )

    if (  ${GTEST_CMAKE} STREQUAL GTEST_CMAKE-NOTFOUND )

        #
        # if GTest sources is not found but downloading allowed...
        #
        if( ${DOWNLOAD_EXTERNALS} )

            #
            # make sure ./externals folder exists and does not contain boost folder
            #
            file( MAKE_DIRECTORY ${external_dir} )
            file( REMOVE_RECURSE ${gtest_dir} )

            #
            # gonna use Git to download Boost from Github
            #
            find_package( Git )
            if ( NOT Git_FOUND )
                message( FATAL_ERROR "Unable to locate Git package!" )
            endif()

            #
            # download GTest
            #
            execute_process(
                COMMAND ${GIT_EXECUTABLE} clone --recurse-submodules https://github.com/google/googletest
                WORKING_DIRECTORY ${external_dir}
                TIMEOUT 14400
                RESULT_VARIABLE git_result
            )

            if ( NOT git_result EQUAL 0 )
                message( FATAL_ERROR "Unable to download GTest library!" )
            endif()

        else()

            #
            # nothing we can do
            #
            message( FATAL_ERROR "\nUnable to locate GTest libraries!\nMake sure it presents on the host and GTEST_ROOT environment variable points actual location\nOR\nuse -DDOWNLOAD_EXTERNALS=ON option to deploy it automatically...\n" )
        
        endif()

    endif()

    #
    # suppress GMock project
    #
    option( BUILD_GMOCK OFF )

    #
    # suppress testing of GTest 
    #
    option( INSTALL_GTEST OFF )

    #
    # add GTest project
    #
    add_subdirectory( ${gtest_dir} )

    #
    # for multiple-configuration place GTest targets to 'externals' folder
    #
    set_target_properties( gtest gtest_main PROPERTIES FOLDER externals )

endif()

include( GoogleTest )