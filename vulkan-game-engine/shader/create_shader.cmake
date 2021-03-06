
foreach ( NX_INCLUDE ${NX_SHADER_INCLUDES} )
    set( SHADER_INCLUDES "${SHADER_INCLUDES}#include \"${NX_INCLUDE}\"\n" )
endforeach()

foreach ( NX_VERTEX_SHADER_ENUM ${NX_VERTEX_SHADER_ENUMS} )
    set( VERTEX_SHADER_ENUMS "${VERTEX_SHADER_ENUMS}\t\t${NX_VERTEX_SHADER_ENUM},\n" )
endforeach()

foreach ( NX_FRAGMENT_SHADER_ENUM ${NX_FRAGMENT_SHADER_ENUMS} )
    set( FRAGMENT_SHADER_ENUMS "${FRAGMENT_SHADER_ENUMS}\t\t${NX_FRAGMENT_SHADER_ENUM},\n" )
endforeach()

foreach ( NX_COMPUTE_SHADER_ENUM ${NX_COMPUTE_SHADER_ENUMS} )
    set( COMPUTE_SHADER_ENUMS "${COMPUTE_SHADER_ENUMS}\t\t${NX_COMPUTE_SHADER_ENUM},\n" )
endforeach()

message( STATUS "configure shader.hpp" )
configure_file( "${INPUT_FILE}" "${OUTPUT_FILE}" )