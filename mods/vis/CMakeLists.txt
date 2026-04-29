project(vis)

add_library(vis MODULE main.cpp)

set_target_properties(vis PROPERTIES
    OUTPUT_NAME "ashww.bardak.vis.raylib"
    PREFIX ""
    SUFFIX ".mod"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
)

target_compile_definitions(vis PRIVATE
    VIS_TILESET_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tilesheet.png"
)

find_package(Threads REQUIRED)

find_package(raylib QUIET)

if (raylib_FOUND)
    target_link_libraries(vis PRIVATE raylib)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(raylib REQUIRED IMPORTED_TARGET GLOBAL raylib)
    target_link_libraries(vis PRIVATE PkgConfig::raylib)
endif()

target_link_libraries(vis PRIVATE
    Threads::Threads
    modlib
    modlib-ifc
)
