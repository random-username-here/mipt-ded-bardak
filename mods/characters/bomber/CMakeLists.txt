project(bomber)

include("${CMAKE_CURRENT_SOURCE_DIR}/proto/CharacterPanProto.cmake")

add_library(bomber MODULE main.cpp "${BOMBER_PROTO_FILE}")

add_custom_target(generate_bomber_mod_proto DEPENDS "${BOMBER_PROTO_FILE}")
add_dependencies(bomber generate_bomber_mod_proto)

target_include_directories(bomber PRIVATE "${BOMBER_PROTO_GEN_DIR}" inc .. ../common)
target_link_libraries(bomber PRIVATE modlib modlib-ifc map-core binmsg libpan_cxx_macros)
target_compile_definitions(bomber PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
set_target_properties(bomber PROPERTIES OUTPUT_NAME "dinichthys.bardak.uctl.bomber")
set_target_properties(bomber PROPERTIES PREFIX "")
set_target_properties(bomber PROPERTIES SUFFIX ".mod")
