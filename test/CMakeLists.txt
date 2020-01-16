cmake_minimum_required( VERSION 3.15 FATAL_ERROR )

add_executable( regression test1.cpp )
target_link_libraries( regression PRIVATE gtest gtest_main )
gtest_add_tests( TARGET regression AUTO )
