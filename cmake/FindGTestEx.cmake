set( external_dir ${PROJECT_SOURCE_DIR}/externals )
set( gtest_dir ${external_dir}/googletest )

find_package( GTest )

if ( GTEST_FOUND )

    add_library( GTestEx INTERFACE )
    target_include_directories( GTestEx INTERFACE GTEST_INCLUDE_DIRS )
    target_link_libraries( GTestEx INTERFACE GTEST_MAIN_LIBRARIES )

elseif ( EXISTS ${gtest_dir}/CMakeLists.txt )

    option( BUILD_GMOCK OFF )
    option( INSTALL_GTEST OFF  )
    add_subdirectory( ${gtest_dir} )
    set_target_properties( gtest gtest_main PROPERTIES FOLDER externals )
    add_library( GTestEx ALIAS gtest_main )

elseif ( ${DOWNLOAD_EXTERNALS} )

    file( MAKE_DIRECTORY ${external_dir} )
    file( REMOVE_RECURSE ${gtest_dir} )

    find_package( Git )
    if ( NOT Git_FOUND )
        message( FATAL_ERROR "Unable to locate Git package!" )
    endif()

    execute_process(
        COMMAND ${GIT_EXECUTABLE} clone --recurse-submodules https://github.com/google/googletest
        WORKING_DIRECTORY ${external_dir}
        TIMEOUT 14400
        RESULT_VARIABLE git_result
    )

    if ( NOT git_result EQUAL 0 )
        message( FATAL_ERROR "Unable to download GoogleTest library!" )
    endif()

    option( BUILD_GMOCK OFF )
    option( INSTALL_GTEST OFF  )
    add_subdirectory( ${gtest_dir} )
    set_target_properties( gtest gtest_main PROPERTIES FOLDER externals )
    add_library( GTestEx ALIAS gtest_main )

else()

    message( FATAL_ERROR "Unable to locate GoogleTest library. Make sure it presents on the host and GTEST_ROOT environment variable points actual location OR use -DDOWNLOAD_EXTERNALS=ON option to deploy it automatically..." )

endif()