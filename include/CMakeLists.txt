cmake_minimum_required( VERSION 3.9 )

get_property( isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG )

if ( isMultiConfig )

    #
    # if IDE is used create a non-compilable target to list header files
    #
    file( GLOB header_files jb/*.h )
    add_library( jb jb-ide.cpp ${header_files} )
    target_compile_features( jb PUBLIC cxx_std_17 )
    target_include_directories( jb PUBLIC . ${Boost_INCLUDE_DIRS} )
    target_link_directories( jb PUBLIC ${Boost_LIBRARY_DIRS} )

else()

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

endif()
