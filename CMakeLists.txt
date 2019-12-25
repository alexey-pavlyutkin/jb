cmake_minimum_required( VERSION 3.9 FATAL_ERROR )

project( jb CXX )

set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

set_property( GLOBAL PROPERTY USE_FOLDERS ON )

add_subdirectory( include )

option( BUILD_REGRESSION "Build regression tests" OFF )
option( BUILD_BENCHMARK  "Build benchmark tests" OFF )

if ( BUILD_REGRESSION )

    find_package( GTestEx )

    enable_testing()
    add_subdirectory( test )

endif()

if ( BUILD_REGRESSION )

    option( BENCHMARK_ENABLE_TESTING OFF )
    add_subdirectory( externals/benchmark )
    set_target_properties( benchmark benchmark_main PROPERTIES FOLDER externals )

endif()
