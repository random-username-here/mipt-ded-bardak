cmake_minimum_required(VERSION 3.10)
project(paladin_client)

set(GENERATED_PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto/generated")

set(PALADIN_PROTO_OUTPUTS "")

function(generate_pan_proto source output_name)
    set(output "${GENERATED_PROTO_DIR}/${output_name}")

    add_custom_command(
        OUTPUT "${output}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GENERATED_PROTO_DIR}"
        COMMAND $<TARGET_FILE:pan> "${source}" "${output}"
        DEPENDS "${source}" pan
        COMMENT "Generating paladin proto header: ${output_name}"
        VERBATIM
    )

    set(current_list ${PALADIN_PROTO_OUTPUTS})
    list(APPEND current_list "${output}")
    set(PALADIN_PROTO_OUTPUTS ${current_list} PARENT_SCOPE)
endfunction()

generate_pan_proto("${CMAKE_CURRENT_SOURCE_DIR}/proto/paladin.pan" paladin_proto.hpp)

add_executable(paladin_client script.cpp ${PALADIN_PROTO_OUTPUTS})

target_include_directories(paladin_client PRIVATE
    "${GENERATED_PROTO_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto"
    "${CMAKE_SOURCE_DIR}/clients/common"
)
target_link_libraries(paladin_client PRIVATE client-core)

add_custom_target(generate_paladin_headers DEPENDS ${PALADIN_PROTO_OUTPUTS})
add_dependencies(paladin_client generate_paladin_headers)
