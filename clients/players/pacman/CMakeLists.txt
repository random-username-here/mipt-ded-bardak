project(pacman_client)

add_executable(pacman_client script.cpp)
target_link_libraries(pacman_client utils)
