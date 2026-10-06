find_package(PkgConfig)
pkg_check_modules(raylib REQUIRED IMPORTED_TARGET GLOBAL raylib)

pan_header(common/proto.pan common/proto.hpp)

add_executable(game)
target_sources(game PRIVATE
    client/main.cpp
    client/debug_wins.cpp
    client/gameloop.cpp
    common/proto.hpp
)
target_include_directories(game PUBLIC client/ common/)
target_link_libraries(
    game PRIVATE 
    PkgConfig::raylib rlimgui msva.client aixlog syncv.client
)

add_library(game.server SHARED)
target_link_libraries(game.server PUBLIC aixlog msva.server modlib syncv.server hmmath)
target_include_directories(game.server PUBLIC server/ common/)
target_sources(game.server PRIVATE server/server.cpp common/proto.hpp)
