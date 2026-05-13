project(viz-animated)

add_library(viz-animated MODULE mod.cpp)
target_link_libraries(viz-animated PRIVATE modlib modlib-ifc map-core)
target_include_directories(viz-animated PRIVATE
    "${CMAKE_SOURCE_DIR}/mods/characters/common"
)
set_target_properties(viz-animated PROPERTIES OUTPUT_NAME "isd.bardak.anim-viz" PREFIX "" SUFFIX ".mod")

find_package(raylib QUIET)
if (raylib_FOUND)
    target_link_libraries(viz-animated PRIVATE raylib)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(raylib REQUIRED IMPORTED_TARGET GLOBAL raylib)
    target_link_libraries(viz-animated PRIVATE PkgConfig::raylib)
endif()


