project(ghost_client)

add_executable(ghost_client script.cpp)
target_link_libraries(ghost_client PRIVATE utils)
