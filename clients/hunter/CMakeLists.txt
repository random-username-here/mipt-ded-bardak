cmake_minimum_required(VERSION 3.10)
project(hunter_client)

set(GENERATED_PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto/generated")

set(HUNTER_PROTO_OUTPUTS "")

function(generate_pan_proto source output_name)
    set(output "${GENERATED_PROTO_DIR}/${output_name}")

    add_custom_command(
        OUTPUT "${output}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GENERATED_PROTO_DIR}"
        COMMAND $<TARGET_FILE:pan> "${source}" "${output}"
        DEPENDS "${source}" pan
        COMMENT "Generating hunter proto header: ${output_name}"
        VERBATIM
    )

    set(current_list ${HUNTER_PROTO_OUTPUTS})
    list(APPEND current_list "${output}")
    set(HUNTER_PROTO_OUTPUTS ${current_list} PARENT_SCOPE)
endfunction()

generate_pan_proto("${CMAKE_CURRENT_SOURCE_DIR}/proto/hunter.pan" hunter_proto.hpp)

add_executable(hunter_client script.cpp ${HUNTER_PROTO_OUTPUTS})

target_include_directories(hunter_client PRIVATE
    "${GENERATED_PROTO_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto"
    "${CMAKE_SOURCE_DIR}/clients/common"
)

target_link_libraries(hunter_client PRIVATE client-core)

add_custom_target(generate_hunter_headers DEPENDS ${HUNTER_PROTO_OUTPUTS})
add_dependencies(hunter_client generate_hunter_headers)
