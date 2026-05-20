cmake_minimum_required(VERSION 3.10)
project(adapter_client)

set(GENERATED_PROTO_DIR "${CMAKE_SOURCE_DIR}/clients/knight/proto/generated")

set(KNIGHT_PROTO_OUTPUTS "")

function(generate_pan_proto source output_name)
    set(output "${GENERATED_PROTO_DIR}/${output_name}")

    add_custom_command(
        OUTPUT "${output}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GENERATED_PROTO_DIR}"
        COMMAND $<TARGET_FILE:pan> "${source}" "${output}"
        DEPENDS "${source}" pan
        COMMENT "Generating knight proto header: ${output_name}"
        VERBATIM
    )

    set(current_list ${KNIGHT_PROTO_OUTPUTS})
    list(APPEND current_list "${output}")
    set(KNIGHT_PROTO_OUTPUTS ${current_list} PARENT_SCOPE)
endfunction()

generate_pan_proto("${CMAKE_SOURCE_DIR}/clients/knight/proto/knight.pan" knight_proto.hpp)

add_executable(adapter_client
    main.cpp
    brain_bridge.cpp
    knight_script_adapter.cpp
    ${KNIGHT_PROTO_OUTPUTS}
)

target_include_directories(adapter_client PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${GENERATED_PROTO_DIR}"
    "${CMAKE_SOURCE_DIR}/clients/common"
)

target_link_libraries(adapter_client PRIVATE client-core generated_proto)
if (WIN32)
    target_link_libraries(adapter_client PRIVATE ws2_32)
endif()

add_custom_target(generate_adapter_headers DEPENDS ${KNIGHT_PROTO_OUTPUTS})
add_dependencies(adapter_client generate_adapter_headers)
