set(PERSON_PROTO_GEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto/generated")
set(PERSON_PROTO_FILE "${PERSON_PROTO_GEN_DIR}/person_proto.hpp")
set(PERSON_PAN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/proto/person.pan")

add_custom_command(
	OUTPUT "${PERSON_PROTO_FILE}"
	COMMAND ${CMAKE_COMMAND} -E make_directory "${PERSON_PROTO_GEN_DIR}"
	COMMAND $<TARGET_FILE:pan> "${PERSON_PAN_SOURCE}" "${PERSON_PROTO_FILE}"
	DEPENDS "${PERSON_PAN_SOURCE}" pan
	COMMENT "Generating person_proto.hpp (person plugin)"
	VERBATIM
)
