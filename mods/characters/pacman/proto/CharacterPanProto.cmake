set(PACMAN_PROTO_GEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto/generated")
set(PACMAN_PROTO_FILE "${PACMAN_PROTO_GEN_DIR}/pacman_proto.hpp")
set(PACMAN_PAN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/proto/pacman.pan")

add_custom_command(
	OUTPUT "${PACMAN_PROTO_FILE}"
	COMMAND ${CMAKE_COMMAND} -E make_directory "${PACMAN_PROTO_GEN_DIR}"
	COMMAND $<TARGET_FILE:pan> "${PACMAN_PAN_SOURCE}" "${PACMAN_PROTO_FILE}"
	DEPENDS "${PACMAN_PAN_SOURCE}" pan
	COMMENT "Generating pacman_proto.hpp (pacman plugin)"
	VERBATIM
)
