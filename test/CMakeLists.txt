cmake_minimum_required( VERSION 3.15 FATAL_ERROR )

file( GLOB test_sources *.cpp )

add_executable( regression ${test_sources} )
target_link_libraries( regression PRIVATE jb )

if ( MSVC AND GTEST_FOUND )

    set_property( TARGET regression PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" )
    target_link_libraries( regression PRIVATE GTest::GTest GTest::Main )

elseif()

    target_link_libraries( regression PRIVATE gtest gtest_main )

endif()

gtest_add_tests( TARGET regression AUTO )
