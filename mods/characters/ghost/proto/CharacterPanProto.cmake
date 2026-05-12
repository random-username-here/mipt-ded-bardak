# Pan codegen for the ghost plugin (this directory only).

set(GHOST_PROTO_GEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto/generated")
set(GHOST_PROTO_FILE "${GHOST_PROTO_GEN_DIR}/ghost_proto.hpp")
set(GHOST_PAN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/proto/ghost.pan")

add_custom_command(
	OUTPUT "${GHOST_PROTO_FILE}"
	COMMAND ${CMAKE_COMMAND} -E make_directory "${GHOST_PROTO_GEN_DIR}"
	COMMAND $<TARGET_FILE:pan> "${GHOST_PAN_SOURCE}" "${GHOST_PROTO_FILE}"
	DEPENDS "${GHOST_PAN_SOURCE}" pan
	COMMENT "Generating ghost_proto.hpp (ghost plugin)"
	VERBATIM
)
