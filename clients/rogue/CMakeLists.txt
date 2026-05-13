cmake_minimum_required(VERSION 3.10)
project(rogue_client)

set(GENERATED_PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto/generated")

set(ROGUE_PROTO_OUTPUTS "")

function(generate_pan_proto source output_name)
    set(output "${GENERATED_PROTO_DIR}/${output_name}")

    add_custom_command(
        OUTPUT "${output}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GENERATED_PROTO_DIR}"
        COMMAND $<TARGET_FILE:pan> "${source}" "${output}"
        DEPENDS "${source}" pan
        COMMENT "Generating rogue proto header: ${output_name}"
        VERBATIM
    )

    set(current_list ${ROGUE_PROTO_OUTPUTS})
    list(APPEND current_list "${output}")
    set(ROGUE_PROTO_OUTPUTS ${current_list} PARENT_SCOPE)
endfunction()

generate_pan_proto("${CMAKE_CURRENT_SOURCE_DIR}/proto/rogue.pan" rogue_proto.hpp)

add_executable(rogue_client script.cpp ${ROGUE_PROTO_OUTPUTS})

target_include_directories(rogue_client PRIVATE
    "${GENERATED_PROTO_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto"
    "${CMAKE_SOURCE_DIR}/clients/common"
)

target_link_libraries(rogue_client PRIVATE client-core)

add_custom_target(generate_rogue_headers DEPENDS ${ROGUE_PROTO_OUTPUTS})
add_dependencies(rogue_client generate_rogue_headers)
