project(map)

add_library(map MODULE 
    Entity.cpp
    Tile.cpp
    Level.cpp
)

target_link_libraries(map PRIVATE modlib modlib-ifc binmsg)

set_target_properties(map PROPERTIES OUTPUT_NAME "ida.bardak.map")
set_target_properties(map PROPERTIES PREFIX "")
set_target_properties(map PROPERTIES SUFFIX ".mod")

