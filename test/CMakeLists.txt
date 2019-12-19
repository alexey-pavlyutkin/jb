cmake_minimum_required( VERSION 3.9 FATAL_ERROR )

add_executable( regression )
find_package(GTest REQUIRED)
target_link_libraries(regression PRIVATE GTest::GTest)
target_link_libraries( regression storageInterface )
