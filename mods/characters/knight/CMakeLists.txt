project(knight)

include("${CMAKE_CURRENT_SOURCE_DIR}/proto/CharacterPanProto.cmake")

add_library(knight MODULE main.cpp "${KNIGHT_PROTO_FILE}")

add_custom_target(generate_knight_mod_proto DEPENDS "${KNIGHT_PROTO_FILE}")
add_dependencies(knight generate_knight_mod_proto)

target_include_directories(knight PRIVATE "${KNIGHT_PROTO_GEN_DIR}" inc ..)
target_link_libraries(knight PRIVATE modlib modlib-ifc map-core binmsg libpan_cxx_macros)
target_compile_definitions(knight PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
set_target_properties(knight PROPERTIES OUTPUT_NAME "ashww.bardak.uctl.knight")
set_target_properties(knight PROPERTIES PREFIX "")
set_target_properties(knight PROPERTIES SUFFIX ".mod")
