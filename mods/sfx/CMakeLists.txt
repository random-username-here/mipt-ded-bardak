project(sfx)

add_library(sfx MODULE main.cpp)

target_include_directories(sfx PRIVATE
    "${CMAKE_SOURCE_DIR}/mods/characters/mage/inc"
)

target_link_libraries(sfx PRIVATE modlib modlib-ifc map-core binmsg)
target_compile_definitions(sfx PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")

set_target_properties(sfx PROPERTIES OUTPUT_NAME "ashww.bardak.sfx" PREFIX "" SUFFIX ".mod")
