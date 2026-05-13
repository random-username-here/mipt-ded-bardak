set(ROGUE_PROTO_GEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto/generated")
set(ROGUE_PROTO_FILE "${ROGUE_PROTO_GEN_DIR}/rogue_proto.hpp")
set(ROGUE_PAN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/proto/rogue.pan")

add_custom_command(
    OUTPUT "${ROGUE_PROTO_FILE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ROGUE_PROTO_GEN_DIR}"
    COMMAND $<TARGET_FILE:pan> "${ROGUE_PAN_SOURCE}" "${ROGUE_PROTO_FILE}"
    DEPENDS "${ROGUE_PAN_SOURCE}" pan
    COMMENT "Generating rogue_proto.hpp (rogue plugin)"
    VERBATIM
)
