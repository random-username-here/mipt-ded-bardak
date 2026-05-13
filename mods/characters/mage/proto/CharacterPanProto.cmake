set(MAGE_PROTO_GEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto/generated")
set(MAGE_PROTO_FILE "${MAGE_PROTO_GEN_DIR}/mage_proto.hpp")
set(MAGE_PAN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/proto/mage.pan")

add_custom_command(
    OUTPUT "${MAGE_PROTO_FILE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MAGE_PROTO_GEN_DIR}"
    COMMAND $<TARGET_FILE:pan> "${MAGE_PAN_SOURCE}" "${MAGE_PROTO_FILE}"
    DEPENDS "${MAGE_PAN_SOURCE}" pan
    COMMENT "Generating mage_proto.hpp (mage plugin)"
    VERBATIM
)
