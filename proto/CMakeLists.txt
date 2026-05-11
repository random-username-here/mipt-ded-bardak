project(generated_proto)

set(GENERATED_PROTO_DIR ${CMAKE_SOURCE_DIR}/proto/generated)
file(MAKE_DIRECTORY ${GENERATED_PROTO_DIR})

function(generate_pan_proto source output_name)
	set(output ${GENERATED_PROTO_DIR}/${output_name})
	add_custom_command(
		OUTPUT ${output}
		COMMAND $<TARGET_FILE:pan> ${CMAKE_SOURCE_DIR}/${source} ${output}
		DEPENDS ${CMAKE_SOURCE_DIR}/${source} pan
	)
	set(GENERATED_PROTO_HEADERS ${GENERATED_PROTO_HEADERS} ${output} PARENT_SCOPE)
endfunction()

generate_pan_proto(specs/proto/srv.pan srv_proto.hpp)
generate_pan_proto(specs/proto/role.pan role_proto.hpp)
generate_pan_proto(specs/proto/pacman.pan pacman_proto.hpp)
generate_pan_proto(specs/proto/ghost.pan ghost_proto.hpp)
generate_pan_proto(specs/proto/person.pan person_proto.hpp)
generate_pan_proto(specs/proto/gmap.pan gmap_proto.hpp)
generate_pan_proto(specs/proto/turns.pan turns_proto.hpp)

add_custom_target(generate_proto_headers DEPENDS ${GENERATED_PROTO_HEADERS})

add_library(generated_proto INTERFACE)
add_dependencies(generated_proto generate_proto_headers)
target_include_directories(generated_proto INTERFACE ${GENERATED_PROTO_DIR})
target_link_libraries(generated_proto INTERFACE libpan_cxx_macros)
