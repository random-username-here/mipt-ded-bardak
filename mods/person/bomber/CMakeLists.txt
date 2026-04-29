project(bomber)

pan_header(../../../specs/proto/bomber.pan bomber_proto.hpp)

add_library(bomber MODULE main.cpp bomber_proto.hpp)
target_link_libraries(bomber PRIVATE modlib modlib-ifc binmsg libpan_cxx_macros)
set_target_properties(bomber PROPERTIES OUTPUT_NAME "dinichthys.bardak.uctl.bomber")
set_target_properties(bomber PROPERTIES PREFIX "")
set_target_properties(bomber PROPERTIES SUFFIX ".mod")
