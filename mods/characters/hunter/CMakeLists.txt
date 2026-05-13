project(hunter)

include("${CMAKE_CURRENT_SOURCE_DIR}/proto/CharacterPanProto.cmake")

add_library(hunter MODULE main.cpp "${HUNTER_PROTO_FILE}")

add_custom_target(generate_hunter_mod_proto DEPENDS "${HUNTER_PROTO_FILE}")
add_dependencies(hunter generate_hunter_mod_proto)

target_include_directories(hunter PRIVATE "${HUNTER_PROTO_GEN_DIR}" inc .. ../common)
target_link_libraries(hunter PRIVATE modlib modlib-ifc map-core binmsg libpan_cxx_macros)
target_compile_definitions(hunter PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
set_target_properties(hunter PROPERTIES OUTPUT_NAME "nellor.bardak.uctl.hunter")
set_target_properties(hunter PROPERTIES PREFIX "")
set_target_properties(hunter PROPERTIES SUFFIX ".mod")
