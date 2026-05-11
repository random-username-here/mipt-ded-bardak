project(pacman)

add_library(pacman MODULE main.cpp)
target_link_libraries(pacman PRIVATE modlib modlib-ifc binmsg generated_proto player_base)
set_target_properties(pacman PROPERTIES OUTPUT_NAME "sevsol.bardak.uctl.pacman")
set_target_properties(pacman PROPERTIES PREFIX "")
set_target_properties(pacman PROPERTIES SUFFIX ".mod")
