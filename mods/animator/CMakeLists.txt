project(animator)

add_library(animator MODULE mod.cpp)
target_link_libraries(animator PRIVATE modlib modlib-ifc)
set_target_properties(animator PROPERTIES OUTPUT_NAME "isd.bardak.animator" PREFIX "" SUFFIX ".mod")
