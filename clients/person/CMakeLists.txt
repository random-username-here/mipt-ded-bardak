cmake_minimum_required(VERSION 3.10)
project(person_client)

set(GENERATED_PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto/generated")

set(PERSON_PROTO_OUTPUTS "")

function(generate_pan_proto source output_name)
    set(output "${GENERATED_PROTO_DIR}/${output_name}")

    add_custom_command(
        OUTPUT "${output}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GENERATED_PROTO_DIR}"
        COMMAND $<TARGET_FILE:pan> "${source}" "${output}"
        DEPENDS "${source}" pan
        COMMENT "Generating person proto header: ${output_name}"
        VERBATIM
    )

    set(current_list ${PERSON_PROTO_OUTPUTS})
    list(APPEND current_list "${output}")
    set(PERSON_PROTO_OUTPUTS ${current_list} PARENT_SCOPE)
endfunction()

generate_pan_proto("${CMAKE_CURRENT_SOURCE_DIR}/proto/person.pan" person_proto.hpp)

add_executable(person_client script.cpp ${PERSON_PROTO_OUTPUTS})

target_include_directories(person_client PRIVATE
    "${GENERATED_PROTO_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto"
)
target_link_libraries(person_client PRIVATE client-core)

add_custom_target(generate_person_headers DEPENDS ${PERSON_PROTO_OUTPUTS})
add_dependencies(person_client generate_person_headers)
