project(lobby)

add_library(lobby MODULE main.cpp)
target_link_libraries(lobby PRIVATE modlib modlib-ifc map-core generated_proto msva-internals)
set_target_properties(lobby PROPERTIES OUTPUT_NAME "ashww.bardak.lobby" PREFIX "" SUFFIX ".mod")
