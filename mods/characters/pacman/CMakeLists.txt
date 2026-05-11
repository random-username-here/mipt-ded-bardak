project(pacman)

include("${CMAKE_CURRENT_SOURCE_DIR}/proto/CharacterPanProto.cmake")

add_library(pacman MODULE main.cpp "${PACMAN_PROTO_FILE}")

add_custom_target(generate_pacman_mod_proto DEPENDS "${PACMAN_PROTO_FILE}")
add_dependencies(pacman generate_pacman_mod_proto)

target_include_directories(pacman PRIVATE "${PACMAN_PROTO_GEN_DIR}" inc ..)
target_link_libraries(pacman PRIVATE modlib modlib-ifc map-core binmsg libpan_cxx_macros)
target_compile_definitions(pacman PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
set_target_properties(pacman PROPERTIES OUTPUT_NAME "sevsol.bardak.uctl.pacman")
set_target_properties(pacman PROPERTIES PREFIX "")
set_target_properties(pacman PROPERTIES SUFFIX ".mod")
