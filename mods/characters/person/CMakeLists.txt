project(person)

pan_header(../../../specs/proto/person.pan ./person_proto.hpp)

add_library(person MODULE main.cpp person_proto.hpp)
target_link_libraries(person PRIVATE modlib modlib-ifc binmsg libpan_cxx_macros)
set_target_properties(person PROPERTIES OUTPUT_NAME "isd.bardak.uctl.person")
set_target_properties(person PROPERTIES PREFIX "")
set_target_properties(person PROPERTIES SUFFIX ".mod")
