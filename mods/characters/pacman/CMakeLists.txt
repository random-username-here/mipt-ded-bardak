project(pacman)

pan_header(../../../specs/proto/pacman.pan pacman_proto.hpp)

add_library(pacman MODULE main.cpp pacman_proto.hpp)
target_link_libraries(pacman PRIVATE modlib modlib-ifc binmsg libpan_cxx_macros)
set_target_properties(pacman PROPERTIES OUTPUT_NAME "sevsol.bardak.uctl.pacman")
set_target_properties(pacman PROPERTIES PREFIX "")
set_target_properties(pacman PROPERTIES SUFFIX ".mod")
