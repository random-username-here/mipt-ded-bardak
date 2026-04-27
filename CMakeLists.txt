cmake_minimum_required(VERSION 3.10)
project(bardak)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for IDE-s
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # for .so-s

add_subdirectory(modlib)
add_subdirectory(specs/binmsg)
add_subdirectory(modlib-ifc)
add_subdirectory(tools/pan)
add_subdirectory(servers/msva)
add_subdirectory(mods/person)
add_subdirectory(mods/timer)
add_subdirectory(mods/map)

add_subdirectory(clients/random)