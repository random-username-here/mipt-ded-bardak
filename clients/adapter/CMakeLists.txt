cmake_minimum_required(VERSION 3.10)
project(adapter_client)

set(GENERATED_PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto/generated")

set(ARCHER_PROTO_OUTPUTS "")

function(generate_pan_proto source output_name)
    set(output "${GENERATED_PROTO_DIR}/${output_name}")

    add_custom_command(
        OUTPUT "${output}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GENERATED_PROTO_DIR}"
        COMMAND $<TARGET_FILE:pan> "${source}" "${output}"
        DEPENDS "${source}" pan
        COMMENT "Generating adapter proto header: ${output_name}"
        VERBATIM
    )

    set(current_list ${ARCHER_PROTO_OUTPUTS})
    list(APPEND current_list "${output}")
    set(ARCHER_PROTO_OUTPUTS ${current_list} PARENT_SCOPE)
endfunction()

generate_pan_proto("${CMAKE_CURRENT_SOURCE_DIR}/proto/adapter.pan" adapter_proto.hpp)

add_executable(adapter_client main.cpp ${ARCHER_PROTO_OUTPUTS})

target_include_directories(adapter_client PRIVATE
    "${GENERATED_PROTO_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto"
    "${CMAKE_SOURCE_DIR}/clients/common"
)

target_link_libraries(adapter_client PRIVATE client-core)

add_custom_target(generate_adapter_headers DEPENDS ${ARCHER_PROTO_OUTPUTS})
add_dependencies(adapter_client generate_adapter_headers)
