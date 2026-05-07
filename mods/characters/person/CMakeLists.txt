project(person)

pan_header(../../../specs/proto/person.pan ./person_proto.hpp)

add_library(person MODULE main.cpp person_proto.hpp)

target_include_directories(person PRIVATE inc ..)
target_link_libraries(person PRIVATE modlib modlib-ifc map-core binmsg libpan_cxx_macros)
target_compile_definitions(person PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
set_target_properties(person PROPERTIES OUTPUT_NAME "isd.bardak.uctl.person")
set_target_properties(person PROPERTIES PREFIX "")
set_target_properties(person PROPERTIES SUFFIX ".mod")
