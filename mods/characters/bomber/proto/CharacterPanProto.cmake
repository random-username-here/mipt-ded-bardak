set(BOMBER_PROTO_GEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto/generated")
set(BOMBER_PROTO_FILE "${BOMBER_PROTO_GEN_DIR}/bomber_proto.hpp")
set(BOMBER_PAN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/proto/bomber.pan")

add_custom_command(
    OUTPUT "${BOMBER_PROTO_FILE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BOMBER_PROTO_GEN_DIR}"
    COMMAND $<TARGET_FILE:pan> "${BOMBER_PAN_SOURCE}" "${BOMBER_PROTO_FILE}"
    DEPENDS "${BOMBER_PAN_SOURCE}" pan
    COMMENT "Generating bomber_proto.hpp (bomber plugin)"
    VERBATIM
)
