cmake_minimum_required(VERSION 3.10)
project(libpan C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(libpan STATIC)
target_sources(libpan PRIVATE src/libpan.c)
target_include_directories(libpan PUBLIC inc/)

add_executable(pan)
target_sources(pan PRIVATE src/standalone.c)
target_link_libraries(pan PRIVATE libpan)

add_library(libpan_cxx_macros INTERFACE)
target_include_directories(libpan_cxx_macros INTERFACE inc/)
target_link_libraries(libpan_cxx_macros INTERFACE binmsg)

# Compilation of examples/add.hpp

function(pan_header source header)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${header}
        COMMAND $<TARGET_FILE:pan> ${CMAKE_CURRENT_SOURCE_DIR}/${source} ${CMAKE_CURRENT_SOURCE_DIR}/${header}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${source} pan
    )
endfunction()

pan_header(examples/add.pan examples/add.hpp)
add_executable(libpan-demo examples/add.hpp examples/add.cpp)
target_link_libraries(libpan-demo PRIVATE libpan_cxx_macros)
