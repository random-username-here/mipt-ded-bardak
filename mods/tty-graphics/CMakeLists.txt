project(tty-graphics)

add_library(ttygraph MODULE main.cpp)
target_link_libraries(ttygraph PRIVATE modlib modlib-ifc msva-internals)
set_target_properties(ttygraph PROPERTIES OUTPUT_NAME "isd.bardak.ttygraph")
set_target_properties(ttygraph PROPERTIES PREFIX "")
set_target_properties(ttygraph PROPERTIES SUFFIX ".mod")

