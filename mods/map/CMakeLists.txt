project(map)

add_library(map MODULE 
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_link_libraries(map PRIVATE modlib modlib-ifc map-core binmsg)
target_compile_definitions(map PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")

set_target_properties(map PROPERTIES OUTPUT_NAME "dodo6b.bardak.map")
set_target_properties(map PROPERTIES PREFIX "")
set_target_properties(map PROPERTIES SUFFIX ".mod")
