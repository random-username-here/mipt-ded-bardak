project(players)

add_library(player_base INTERFACE player_base.hpp)
target_include_directories(player_base INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(ghost)
add_subdirectory(pacman)
