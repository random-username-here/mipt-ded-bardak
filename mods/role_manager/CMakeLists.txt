project(role_manager)

add_library(role_manager MODULE main.cpp)
target_link_libraries(role_manager PRIVATE modlib modlib-ifc binmsg generated_proto)
set_target_properties(role_manager PROPERTIES OUTPUT_NAME "sevsol.bardak.role_mgr")
set_target_properties(role_manager PROPERTIES PREFIX "")
set_target_properties(role_manager PROPERTIES SUFFIX ".mod")
