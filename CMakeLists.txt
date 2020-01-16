cmake_minimum_required( VERSION 3.15 FATAL_ERROR )

if ( MSVC )
    cmake_policy( SET CMP0091 NEW )
endif()


project( jb VERSION 0.1 LANGUAGES CXX )


#
# set path to Cmake modules
#
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake )


#
# allow folders for multi-configs
#
get_property( isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG )
if ( isMultiConfig )
    set_property( GLOBAL PROPERTY USE_FOLDERS ON )
endif()


#
# try find Boost
#
find_package( BoostEx )


#
#
#
add_subdirectory( include )


#
# configurable options for building regression/benchmark tests
#
option( BUILD_JB_TESTS "Build regression tests" ON )
option( BUILD_JB_BENCHMARK  "Build benchmark tests" ON )


#
# if a user selected build regression tests -> add them
#
if ( BUILD_JB_TESTS )
    find_package( GTestEx )
    enable_testing()
    add_subdirectory( test )
endif()


#
# if a user selected build benchmark tests -> add them
#
if ( BUILD_JB_BENCHMARK )
#    option( BENCHMARK_ENABLE_TESTING OFF )
#    add_subdirectory( externals/benchmark )
#    set_target_properties( benchmark benchmark_main PROPERTIES FOLDER externals )
endif()
