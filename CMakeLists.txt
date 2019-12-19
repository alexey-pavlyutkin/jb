cmake_minimum_required( VERSION 3.9 FATAL_ERROR )

project( jb CXX )

add_subdirectory( include )

option( BUILD_TESTS "Build tests" OFF )

if ( BUILD_TESTS AND TARGET storageInterface )

    enable_testing()
    add_subdirectory( test )

endif()
