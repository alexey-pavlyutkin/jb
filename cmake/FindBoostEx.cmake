set( external_dir ${PROJECT_SOURCE_DIR}/externals )
set( boost_dir ${external_dir}/boost )

set( ${BOOST_ROOT} ${boost_dir} )
cmake_policy( SET CMP0074 NEW )
find_package( boost 1.67.0 )

message( "Boost_FOUND = " ${Boost_FOUND} )

if ( Boost_FOUND )

    set( BoostEx_FOUND TRUE PARENT_SCOPE )
    set( BoostEx_Include ${Boost_INCLUDE_DIRS} PARENT_SCOPE )
    set( BoostEx_Lib ${Boost_LIBRARY_DIRS} PARENT_SCOPE )

elseif( ${DOWNLOAD_EXTERNALS} )

    set( temp_dir ${PROJECT_SOURCE_DIR}/temp )
    set( temp_boost_dir ${temp_dir}/boost )
#[[
    file( MAKE_DIRECTORY ${external_dir} )
    file( REMOVE_RECURSE ${temp_dir} )
    file( MAKE_DIRECTORY ${temp_dir} )
    
    find_package( Git )
    message( " Git_FOUND = " Git_FOUND "; GIT_EXECUTABLE = " ${GIT_EXECUTABLE} )

    if ( NOT Git_FOUND )
        set( BoostEx_FOUND FALSE PARENT_SCOPE )
        message( FATAL_ERROR "Unable to locate Git package!" )
    endif()

    execute_process(
        COMMAND ${GIT_EXECUTABLE} clone --recurse-submodules https://github.com/boostorg/boost
        WORKING_DIRECTORY ${temp_dir}
        TIMEOUT 14400
        RESULT_VARIABLE git_result
    )

    if ( NOT git_result EQUAL 0 )
        set( BoostEx_FOUND FALSE PARENT_SCOPE )
        message( FATAL_ERROR "Unable to download Boost library!" )
    endif()
]]
    if ( WIN32 )
        set( boost_bootstrap "bootstrap.bat" )
        #set( boost_builder "b2.exe --build-type=complete install --prefix=${boost_dir}" )
        set( boost_builder "b2.exe" )
        set( boost_build_args "--build-type=complete" )
        set( boost_install_args " install --prefix=${boost_dir}" )
        message( "boost_builder = " ${boost_builder} )
    elseif ( UNIX )
        set( boost_bootstrap "bootstrap.bat" )
        set( boost_builder "b2" )
    else()
        set( BoostEx_FOUND FALSE PARENT_SCOPE )
        message( FATAL_ERROR "Dunno how to build Boost library!" )
    endif()

    execute_process(
        COMMAND ${boost_bootstrap}
        WORKING_DIRECTORY ${temp_boost_dir}
        RESULT_VARIABLE bootstrap_result
    )

    if ( NOT bootstrap_result EQUAL 0 )
        set( BoostEx_FOUND FALSE PARENT_SCOPE )
        message( FATAL_ERROR "Unable to bootstrap Boost library!" )
    endif()

    execute_process(
        COMMAND ${boost_builder} ${boost_builder_args}
        WORKING_DIRECTORY ${temp_boost_dir}
        TIMEOUT 14400
        RESULT_VARIABLE boost_build_result
    )

    if ( NOT boost_build_result EQUAL 0 )
        set( BoostEx_FOUND FALSE PARENT_SCOPE )
        message( FATAL_ERROR "Unable to build Boost library!" )
    endif()

    execute_process(
        COMMAND ${boost_builder} ${boost_install_args}
        WORKING_DIRECTORY ${temp_boost_dir}
        TIMEOUT 14400
        RESULT_VARIABLE boost_install_result
    )

    if ( NOT boost_install_result EQUAL 0 )
        set( BoostEx_FOUND FALSE PARENT_SCOPE )
        message( FATAL_ERROR "Unable to install Boost library!" )
    endif()

    set( BoostEx_FOUND TRUE PARENT_SCOPE )
    set( BoostEx_Include ${boost_dir} PARENT_SCOPE )
    set( BoostEx_Lib ${boost_dir}/stage/lib PARENT_SCOPE )

    file( REMOVE_RECURSE ${temp_dir} )

else()
    set( BoostEx_FOUND FALSE PARENT_SCOPE )
    message( FATAL_ERROR "Unable to locate Boost library. Make sure it presents on the host OR use -DDOWNLOAD_EXTERNALS to deploy it automatically (takes a long time)..." )
endif()
