set(KNIGHT_PROTO_GEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto/generated")
set(KNIGHT_PROTO_FILE "${KNIGHT_PROTO_GEN_DIR}/knight_proto.hpp")
set(KNIGHT_PAN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/proto/knight.pan")

add_custom_command(
    OUTPUT "${KNIGHT_PROTO_FILE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${KNIGHT_PROTO_GEN_DIR}"
    COMMAND $<TARGET_FILE:pan> "${KNIGHT_PAN_SOURCE}" "${KNIGHT_PROTO_FILE}"
    DEPENDS "${KNIGHT_PAN_SOURCE}" pan
    COMMENT "Generating knight_proto.hpp (knight plugin)"
    VERBATIM
)
