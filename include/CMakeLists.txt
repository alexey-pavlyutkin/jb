cmake_minimum_required( VERSION 3.9 )

#
# Create storage jb library
#
# Since the this is PURE TEMPLATE library (i.e. there is nothing to build) create target as
# INTERFACE, specify include path, and dependencies
#
add_library( jb INTERFACE )
target_compile_features( jb INTERFACE cxx_std_17 )
target_include_directories( jb INTERFACE . ${Boost_INCLUDE_DIRS} )
target_link_directories( jb INTERFACE ${Boost_LIBRARY_DIRS} )

#
# if IDE is used create a non-compilable target to list header files
#
get_property( isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG )
if ( isMultiConfig )
    
    file( GLOB header_files jb/*.h )
    add_library( jb-ide STATIC jb-ide.cpp ${header_files} )
    target_link_libraries( jb-ide PRIVATE jb )

endif()
