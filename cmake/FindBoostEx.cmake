cmake_minimum_required( VERSION 3.13 )

set( BoostEx_FOUND FALSE )

set( external_dir ${PROJECT_SOURCE_DIR}/externals )
set( boost_dir ${external_dir}/boost )

set( BOOST_INCLUDEDIR ${boost_dir} )
set( BOOST_LIBRARYDIR ${boost_dir}/stage/lib )

cmake_policy( SET CMP0074 NEW )
find_package( boost 1.67.0 )

if ( Boost_FOUND AND Boost_LIB_VERSION STRGREATER_EQUAL "1.67.0" )

    add_library( boost INTERFACE )
    target_include_directories( boost INTERFACE ${Boost_INCLUDE_DIRS} )
    target_link_directories( boost INTERFACE ${Boost_LIBRARY_DIRS} )

elseif( ${DOWNLOAD_EXTERNALS} )

    file( MAKE_DIRECTORY ${external_dir} )
    file( REMOVE_RECURSE ${boost_dir} )
    
    find_package( Git )
    if ( NOT Git_FOUND )
        message( FATAL_ERROR "Unable to locate Git package!" )
    endif()

    execute_process(
        COMMAND ${GIT_EXECUTABLE} clone --recurse-submodules https://github.com/boostorg/boost
        WORKING_DIRECTORY ${external_dir}
        TIMEOUT 14400
        RESULT_VARIABLE git_result
    )

    if ( NOT git_result EQUAL 0 )
        message( FATAL_ERROR "Unable to download Boost library!" )
    endif()

    if ( WIN32 )
        set( boost_bootstrap "bootstrap.bat" )
        set( boost_builder "b2.exe" )
    elseif ( UNIX )
        set( boost_bootstrap "bootstrap.sh" )
        set( boost_builder "b2" )
    else()
        set( BoostEx_FOUND FALSE PARENT_SCOPE )
        message( FATAL_ERROR "Dunno how to build Boost library!" )
    endif()

    execute_process(
        COMMAND ${boost_bootstrap}
        WORKING_DIRECTORY ${boost_dir}
        RESULT_VARIABLE bootstrap_result
    )

    if ( NOT bootstrap_result EQUAL 0 )
        message( FATAL_ERROR "Unable to bootstrap Boost library!" )
    endif()

    execute_process(
        COMMAND ${boost_builder}
        WORKING_DIRECTORY ${boost_dir}
        TIMEOUT 14400
        RESULT_VARIABLE boost_build_result
    )

    if ( NOT boost_build_result EQUAL 0 )
        message( FATAL_ERROR "Unable to build Boost library!" )
    endif()

    add_library( boost INTERFACE )
    target_include_directories( boost INTERFACE ${BOOST_INCLUDEDIR} )
    target_link_directories( boost INTERFACE ${BOOST_LIBRARYDIR} )

else()
    message( FATAL_ERROR "Unable to locate Boost library. Make sure it presents on the host and BOOST_ROOT environment variable points actual location OR use -DDOWNLOAD_EXTERNALS=ON option to deploy it automatically (takes a long time)..." )
endif()
