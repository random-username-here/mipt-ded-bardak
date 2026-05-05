project(map-core)

add_library(map-core STATIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Entity.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Tile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Level.cpp
)

target_link_libraries(map-core PUBLIC modlib modlib-ifc binmsg)
