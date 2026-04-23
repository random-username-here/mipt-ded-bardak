cmake_minimum_required(VERSION 3.10)
project(bardak)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for IDE-s
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # for .so-s

add_subdirectory(modlib)
add_subdirectory(binmsg)
add_subdirectory(modlib-ifc)
add_subdirectory(pan)
add_subdirectory(msva)
add_subdirectory(mods/person)
add_subdirectory(mods/timer)
