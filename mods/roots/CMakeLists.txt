project(roots)

add_library(roots MODULE main.cpp)
target_link_libraries(roots PRIVATE modlib modlib-ifc map-core binmsg)
target_compile_definitions(roots PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
set_target_properties(roots PROPERTIES OUTPUT_NAME "ashww.bardak.roots" PREFIX "" SUFFIX ".mod")
