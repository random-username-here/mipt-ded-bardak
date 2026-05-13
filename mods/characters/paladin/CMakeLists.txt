project(paladin)

include("${CMAKE_CURRENT_SOURCE_DIR}/proto/CharacterPanProto.cmake")

add_library(paladin MODULE main.cpp "${PALADIN_PROTO_FILE}")

add_custom_target(generate_paladin_mod_proto DEPENDS "${PALADIN_PROTO_FILE}")
add_dependencies(paladin generate_paladin_mod_proto)

target_include_directories(paladin PRIVATE "${PALADIN_PROTO_GEN_DIR}" inc .. ../common)
target_link_libraries(paladin PRIVATE modlib modlib-ifc map-core binmsg libpan_cxx_macros)
target_compile_definitions(paladin PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
set_target_properties(paladin PROPERTIES OUTPUT_NAME "ashww.bardak.uctl.paladin")
set_target_properties(paladin PROPERTIES PREFIX "")
set_target_properties(paladin PROPERTIES SUFFIX ".mod")
