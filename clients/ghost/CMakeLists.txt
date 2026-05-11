cmake_minimum_required(VERSION 3.10)
project(ghost_client)

set(GENERATED_PROTO_DIR ${CMAKE_BINARY_DIR}/proto/generated)
file(MAKE_DIRECTORY ${GENERATED_PROTO_DIR})

set(GHOST_PROTO_OUTPUTS "")

function(generate_pan_proto source output_name)
    set(output ${GENERATED_PROTO_DIR}/${output_name})
    
    add_custom_command(
        OUTPUT ${output}
        COMMAND $<TARGET_FILE:pan> "${source}" "${output}"
        DEPENDS "${source}" pan
        COMMENT "Generating proto header: ${output_name}"
    )
    
    set(GHOST_PROTO_OUTPUTS ${GHOST_PROTO_OUTPUTS} ${output} PARENT_SCOPE)
endfunction()

generate_pan_proto("${CMAKE_CURRENT_SOURCE_DIR}/proto/ghost.pan" ghost_proto.hpp)

add_subdirectory(utils)
add_executable(ghost_client script.cpp ${GHOST_PROTO_OUTPUTS}) 

target_include_directories(ghost_client PRIVATE 
    ${GENERATED_PROTO_DIR}
    proto 
)
target_link_libraries(ghost_client PRIVATE utils)

add_custom_target(generate_ghost_headers DEPENDS ${GHOST_PROTO_OUTPUTS})
add_dependencies(ghost_client generate_ghost_headers)