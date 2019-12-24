cmake_minimum_required( VERSION 3.9 )

find_package( BoostEx )

if ( TARGET boost )

    message( "Boost defined" )

    add_library( storage INTERFACE )

    target_compile_features( storage INTERFACE cxx_std_17 )
    target_include_directories( storage INTERFACE . )
    target_link_libraries( storage INTERFACE boost )

    get_property( isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG )
    if ( isMultiConfig )
        file( GLOB header_files *.h )
        add_custom_target( storage-headers SOURCES ${header_files} )
    endif()

endif()
