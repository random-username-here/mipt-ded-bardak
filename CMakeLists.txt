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

option(BUILD_GRAPHICS "Build graphical modules" ON)

if(BUILD_GRAPHICS)
    add_subdirectory(mods/anim-viz)
    # add_subdirectory(mods/tty-graphics)
endif()


add_subdirectory(third_party/inih)
add_subdirectory(core)
add_subdirectory(specs/binmsg)
add_subdirectory(modlib-ifc)
add_subdirectory(tools/pan)
add_subdirectory(proto)
add_subdirectory(servers/msva)
add_subdirectory(mods/characters)
add_subdirectory(mods/role_manager)
add_subdirectory(mods/timer)
add_subdirectory(mods/map)
add_subdirectory(mods/roots)
add_subdirectory(mods/assets)
add_subdirectory(entity_components)
add_subdirectory(mods/animator)

add_subdirectory(clients/ghost)
add_subdirectory(clients/pacman)
add_subdirectory(clients/person)
add_subdirectory(clients/knight)
add_subdirectory(clients/adapter)
add_subdirectory(clients/rogue)
add_subdirectory(clients/archer)
add_subdirectory(clients/mage)
add_subdirectory(clients/paladin)
add_subdirectory(clients/hunter)
