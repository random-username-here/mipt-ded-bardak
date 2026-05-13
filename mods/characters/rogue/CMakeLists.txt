project(rogue)

include("${CMAKE_CURRENT_SOURCE_DIR}/proto/CharacterPanProto.cmake")

add_library(rogue MODULE main.cpp "${ROGUE_PROTO_FILE}")

add_custom_target(generate_rogue_mod_proto DEPENDS "${ROGUE_PROTO_FILE}")
add_dependencies(rogue generate_rogue_mod_proto)

target_include_directories(rogue PRIVATE "${ROGUE_PROTO_GEN_DIR}" inc .. ../common)
target_link_libraries(rogue PRIVATE modlib modlib-ifc map-core binmsg libpan_cxx_macros)
target_compile_definitions(rogue PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
set_target_properties(rogue PROPERTIES OUTPUT_NAME "ashww.bardak.uctl.rogue")
set_target_properties(rogue PROPERTIES PREFIX "")
set_target_properties(rogue PROPERTIES SUFFIX ".mod")
