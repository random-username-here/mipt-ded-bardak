project(role_manager)

pan_header(../../specs/proto/role.pan role_proto.hpp)

add_library(role_manager MODULE main.cpp role_proto.hpp)
target_link_libraries(role_manager PRIVATE modlib modlib-ifc binmsg libpan_cxx_macros)
set_target_properties(role_manager PROPERTIES OUTPUT_NAME "sevsol.bardak.role_mgr")
set_target_properties(role_manager PROPERTIES PREFIX "")
set_target_properties(role_manager PROPERTIES SUFFIX ".mod")
