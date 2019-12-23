cmake_minimum_required( VERSION 3.9 FATAL_ERROR )

project( jb CXX )

set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

add_subdirectory( include )

option( BUILD_REGRESSION "Build regression tests" OFF )
option( BUILD_BENCHMARK  "Build benchmark tests" OFF )

if ( BUILD_TESTS AND TARGET storageInterface )

    enable_testing()
    #add_subdirectory( test )

endif()
