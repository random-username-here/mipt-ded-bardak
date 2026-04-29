project(ghost)

add_library(ghost MODULE main.cpp)
target_link_libraries(ghost PRIVATE modlib modlib-ifc binmsg generated_proto player_base)
set_target_properties(ghost PROPERTIES OUTPUT_NAME "sevsol.bardak.uctl.ghost")
set_target_properties(ghost PROPERTIES PREFIX "")
set_target_properties(ghost PROPERTIES SUFFIX ".mod")
