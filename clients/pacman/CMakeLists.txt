cmake_minimum_required(VERSION 3.10)
project(pacman_client)

set(GENERATED_PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto/generated")
set(PACMAN_PROTO_OUTPUTS "")

function(generate_pan_proto source output_name)
    set(output "${GENERATED_PROTO_DIR}/${output_name}")
    
    add_custom_command(
        OUTPUT "${output}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GENERATED_PROTO_DIR}"
        COMMAND $<TARGET_FILE:pan> "${source}" "${output}"
        DEPENDS "${source}" pan
        COMMENT "Generating pacman proto header: ${output_name}"
        VERBATIM
    )
    
    set(current_list ${PACMAN_PROTO_OUTPUTS})
    list(APPEND current_list "${output}")
    set(PACMAN_PROTO_OUTPUTS ${current_list} PARENT_SCOPE)
endfunction()

generate_pan_proto("${CMAKE_CURRENT_SOURCE_DIR}/proto/pacman.pan" pacman_proto.hpp)

add_executable(pacman_client script.cpp ${PACMAN_PROTO_OUTPUTS}) 

target_include_directories(pacman_client PRIVATE 
    "${GENERATED_PROTO_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto" 
)
target_link_libraries(pacman_client PRIVATE client-core)

add_custom_target(generate_pacman_headers DEPENDS ${PACMAN_PROTO_OUTPUTS})
add_dependencies(pacman_client generate_pacman_headers)