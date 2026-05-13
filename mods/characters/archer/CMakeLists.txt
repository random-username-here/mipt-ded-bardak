project(archer)

include("${CMAKE_CURRENT_SOURCE_DIR}/proto/CharacterPanProto.cmake")

add_library(archer MODULE main.cpp "${ARCHER_PROTO_FILE}")

add_custom_target(generate_archer_mod_proto DEPENDS "${ARCHER_PROTO_FILE}")
add_dependencies(archer generate_archer_mod_proto)

target_include_directories(archer PRIVATE "${ARCHER_PROTO_GEN_DIR}" inc .. ../common)
target_link_libraries(archer PRIVATE modlib modlib-ifc map-core binmsg libpan_cxx_macros)
target_compile_definitions(archer PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
set_target_properties(archer PROPERTIES OUTPUT_NAME "ashww.bardak.uctl.archer")
set_target_properties(archer PROPERTIES PREFIX "")
set_target_properties(archer PROPERTIES SUFFIX ".mod")
