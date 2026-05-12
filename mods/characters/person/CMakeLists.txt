project(person)

include("${CMAKE_CURRENT_SOURCE_DIR}/proto/CharacterPanProto.cmake")

add_library(person MODULE main.cpp "${PERSON_PROTO_FILE}")

add_custom_target(generate_person_mod_proto DEPENDS "${PERSON_PROTO_FILE}")
add_dependencies(person generate_person_mod_proto)

target_include_directories(person PRIVATE "${PERSON_PROTO_GEN_DIR}" inc ..)
target_link_libraries(person PRIVATE modlib modlib-ifc map-core binmsg libpan_cxx_macros)
target_compile_definitions(person PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
set_target_properties(person PROPERTIES OUTPUT_NAME "isd.bardak.uctl.person")
set_target_properties(person PROPERTIES PREFIX "")
set_target_properties(person PROPERTIES SUFFIX ".mod")
