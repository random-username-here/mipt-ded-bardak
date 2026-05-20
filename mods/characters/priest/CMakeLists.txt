project(priest)

include("${CMAKE_CURRENT_SOURCE_DIR}/proto/CharacterPanProto.cmake")

add_library(priest MODULE main.cpp "${PRIEST_PROTO_FILE}")

add_custom_target(generate_priest_mod_proto DEPENDS "${PRIEST_PROTO_FILE}")
add_dependencies(priest generate_priest_mod_proto)

target_include_directories(priest PRIVATE "${PRIEST_PROTO_GEN_DIR}" inc .. ../common)
target_link_libraries(priest PRIVATE modlib modlib-ifc map-core binmsg libpan_cxx_macros)
target_compile_definitions(priest PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
set_target_properties(priest PROPERTIES OUTPUT_NAME "dodo6b.bardak.uctl.priest")
set_target_properties(priest PROPERTIES PREFIX "")
set_target_properties(priest PROPERTIES SUFFIX ".mod")
