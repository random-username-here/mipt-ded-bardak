set(TANK_PROTO_GEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto/generated")
set(TANK_PROTO_FILE "${TANK_PROTO_GEN_DIR}/tank_proto.hpp")
set(TANK_PAN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/proto/tank.pan")

add_custom_command(
    OUTPUT "${TANK_PROTO_FILE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${TANK_PROTO_GEN_DIR}"
    COMMAND $<TARGET_FILE:pan> "${TANK_PAN_SOURCE}" "${TANK_PROTO_FILE}"
    DEPENDS "${TANK_PAN_SOURCE}" pan
    COMMENT "Generating tank_proto.hpp (tank plugin)"
    VERBATIM
)
