set(PALADIN_PROTO_GEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto/generated")
set(PALADIN_PROTO_FILE "${PALADIN_PROTO_GEN_DIR}/paladin_proto.hpp")
set(PALADIN_PAN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/proto/paladin.pan")

add_custom_command(
    OUTPUT "${PALADIN_PROTO_FILE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PALADIN_PROTO_GEN_DIR}"
    COMMAND $<TARGET_FILE:pan> "${PALADIN_PAN_SOURCE}" "${PALADIN_PROTO_FILE}"
    DEPENDS "${PALADIN_PAN_SOURCE}" pan
    COMMENT "Generating paladin_proto.hpp (paladin plugin)"
    VERBATIM
)
