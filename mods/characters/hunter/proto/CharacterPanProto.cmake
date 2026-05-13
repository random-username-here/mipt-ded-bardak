set(HUNTER_PROTO_GEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto/generated")
set(HUNTER_PROTO_FILE "${HUNTER_PROTO_GEN_DIR}/hunter_proto.hpp")
set(HUNTER_PAN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/proto/hunter.pan")

add_custom_command(
    OUTPUT "${HUNTER_PROTO_FILE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${HUNTER_PROTO_GEN_DIR}"
    COMMAND $<TARGET_FILE:pan> "${HUNTER_PAN_SOURCE}" "${HUNTER_PROTO_FILE}"
    DEPENDS "${HUNTER_PAN_SOURCE}" pan
    COMMENT "Generating hunter_proto.hpp (hunter plugin)"
    VERBATIM
)
