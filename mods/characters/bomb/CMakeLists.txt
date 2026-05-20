project(bomb)

add_library(bomb MODULE main.cpp)
target_link_libraries(bomb PRIVATE modlib modlib-ifc map-core binmsg)
target_compile_definitions(bomb PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
set_target_properties(bomb PROPERTIES OUTPUT_NAME "dinichthys.bardak.bomb" PREFIX "" SUFFIX ".mod")
