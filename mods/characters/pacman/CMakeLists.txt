project(pacman)

include("${CMAKE_CURRENT_SOURCE_DIR}/proto/CharacterPanProto.cmake")

add_library(pacman MODULE main.cpp "${PACMAN_PROTO_FILE}")

add_custom_target(generate_pacman_mod_proto DEPENDS "${PACMAN_PROTO_FILE}")
add_dependencies(pacman generate_pacman_mod_proto)

target_include_directories(pacman PRIVATE "${PACMAN_PROTO_GEN_DIR}")
target_link_libraries(pacman PRIVATE modlib modlib-ifc binmsg generated_proto player_base)
set_target_properties(pacman PROPERTIES OUTPUT_NAME "sevsol.bardak.uctl.pacman")
set_target_properties(pacman PROPERTIES PREFIX "")
set_target_properties(pacman PROPERTIES SUFFIX ".mod")
