project(ghost)

include("${CMAKE_CURRENT_SOURCE_DIR}/proto/CharacterPanProto.cmake")

add_library(ghost MODULE main.cpp "${GHOST_PROTO_FILE}")

add_custom_target(generate_ghost_mod_proto DEPENDS "${GHOST_PROTO_FILE}")
add_dependencies(ghost generate_ghost_mod_proto)

target_include_directories(ghost PRIVATE "${GHOST_PROTO_GEN_DIR}" inc ..)
target_link_libraries(ghost PRIVATE modlib modlib-ifc map-core binmsg libpan_cxx_macros)
target_compile_definitions(ghost PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
set_target_properties(ghost PROPERTIES OUTPUT_NAME "sevsol.bardak.uctl.ghost")
set_target_properties(ghost PROPERTIES PREFIX "")
set_target_properties(ghost PROPERTIES SUFFIX ".mod")
