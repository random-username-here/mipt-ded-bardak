project(ghost)

pan_header(../../../specs/proto/ghost.pan ghost_proto.hpp)

add_library(ghost MODULE main.cpp ghost_proto.hpp)
target_link_libraries(ghost PRIVATE modlib modlib-ifc binmsg libpan_cxx_macros)
set_target_properties(ghost PROPERTIES OUTPUT_NAME "sevsol.bardak.uctl.ghost")
set_target_properties(ghost PROPERTIES PREFIX "")
set_target_properties(ghost PROPERTIES SUFFIX ".mod")
