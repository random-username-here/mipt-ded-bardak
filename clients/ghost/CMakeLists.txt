cmake_minimum_required(VERSION 3.10)
project(ghost_client)

set(GENERATED_PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto/generated")

set(GHOST_PROTO_OUTPUTS "")

function(generate_pan_proto source output_name)
    set(output "${GENERATED_PROTO_DIR}/${output_name}")
    
    add_custom_command(
        OUTPUT "${output}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GENERATED_PROTO_DIR}"
        COMMAND $<TARGET_FILE:pan> "${source}" "${output}"
        DEPENDS "${source}" pan
        COMMENT "Generating ghost proto header: ${output_name}"
        VERBATIM
    )
    
    set(current_list ${GHOST_PROTO_OUTPUTS})
    list(APPEND current_list "${output}")
    set(GHOST_PROTO_OUTPUTS ${current_list} PARENT_SCOPE)
endfunction()

generate_pan_proto("${CMAKE_CURRENT_SOURCE_DIR}/proto/ghost.pan" ghost_proto.hpp)

add_executable(ghost_client script.cpp ${GHOST_PROTO_OUTPUTS}) 

target_include_directories(ghost_client PRIVATE 
    "${GENERATED_PROTO_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto" 
)
target_link_libraries(ghost_client PRIVATE client-core)

add_custom_target(generate_ghost_headers DEPENDS ${GHOST_PROTO_OUTPUTS})
add_dependencies(ghost_client generate_ghost_headers)