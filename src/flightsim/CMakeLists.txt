find_package(PkgConfig)
pkg_check_modules(raylib REQUIRED IMPORTED_TARGET GLOBAL raylib)

add_executable(flightsim)
target_sources(flightsim PRIVATE
    src/main.cpp
    src/debug_wins.cpp
)
target_link_libraries(
    flightsim PRIVATE 
    PkgConfig::raylib rlimgui msva.client aixlog syncv.client
)

