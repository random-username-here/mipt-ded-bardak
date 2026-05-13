project(mage)

include("${CMAKE_CURRENT_SOURCE_DIR}/proto/CharacterPanProto.cmake")

add_library(mage MODULE main.cpp "${MAGE_PROTO_FILE}")

add_custom_target(generate_mage_mod_proto DEPENDS "${MAGE_PROTO_FILE}")
add_dependencies(mage generate_mage_mod_proto)

target_include_directories(mage PRIVATE "${MAGE_PROTO_GEN_DIR}" inc .. ../common)
target_link_libraries(mage PRIVATE modlib modlib-ifc map-core binmsg libpan_cxx_macros)
target_compile_definitions(mage PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
set_target_properties(mage PROPERTIES OUTPUT_NAME "ashww.bardak.uctl.mage")
set_target_properties(mage PROPERTIES PREFIX "")
set_target_properties(mage PROPERTIES SUFFIX ".mod")
