cmake_minimum_required(VERSION 3.10)
project(bardak)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for IDE-s
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # for .so-s

option(SANITIZE "Enable compiler sanitizers" ON)
if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(
        -Wall
        -Wextra
         $<$<BOOL:${SANITIZE}>:-fsanitize=undefined>
    )
    add_link_options(
         $<$<BOOL:${SANITIZE}>:-fsanitize=undefined>
    )
endif()


add_subdirectory(modlib)
add_subdirectory(specs/binmsg)
add_subdirectory(modlib-ifc)
add_subdirectory(tools/pan)
add_subdirectory(servers/msva)
add_subdirectory(mods/characters)
add_subdirectory(mods/role_manager)
add_subdirectory(mods/timer)
add_subdirectory(mods/map)
add_subdirectory(mods/vis)
add_subdirectory(mods/assets)
add_subdirectory(mods/tty-graphics)
add_subdirectory(clients/random)
