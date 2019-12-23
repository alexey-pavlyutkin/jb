cmake_minimum_required( VERSION 3.9 )

message( "CMAKE_MODULE_PATH = " ${CMAKE_MODULE_PATH} )

find_package( BoostEx )

message( "BoostEx_FOUND = " ${BoostEx_FOUND} )
message( "BoostEx_Include = " ${BoostEx_Include} )
message( "BoostEx_Lib = " ${BoostEx_Lib} )

#if ( ${Boost_FOUND} )

#    add_library( storageInterface INTERFACE )

#    target_compile_features( storageInterface INTERFACE cxx_std_17 )
#    target_include_directories( storageInterface INTERFACE . ${Boost_INCLUDE_DIRS} )
#    target_link_directories( storageInterface INTERFACE ${Boost_LIBRARY_DIRS} )

    #get_property( isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG )
    #if ( isMultiConfig )
    #    file( GLOB header_files *.h )
    #    add_custom_target( Storage SOURCES ${header_files} )
    #endif()

#endif()
