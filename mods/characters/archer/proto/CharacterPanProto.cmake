set(ARCHER_PROTO_GEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto/generated")
set(ARCHER_PROTO_FILE "${ARCHER_PROTO_GEN_DIR}/archer_proto.hpp")
set(ARCHER_PAN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/proto/archer.pan")

add_custom_command(
    OUTPUT "${ARCHER_PROTO_FILE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ARCHER_PROTO_GEN_DIR}"
    COMMAND $<TARGET_FILE:pan> "${ARCHER_PAN_SOURCE}" "${ARCHER_PROTO_FILE}"
    DEPENDS "${ARCHER_PAN_SOURCE}" pan
    COMMENT "Generating archer_proto.hpp (archer plugin)"
    VERBATIM
)
