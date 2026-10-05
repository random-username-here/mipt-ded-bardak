cmake_minimum_required(VERSION 3.10)
project(bardak)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for IDE-s
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # for .so-s
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/" ${CMAKE_MODULE_PATH})
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(src/thirdparty)
add_subdirectory(src/modlib)
add_subdirectory(src/pan)
add_subdirectory(src/msva)
add_subdirectory(src/syncv)
add_subdirectory(src/flightsim-server)
add_subdirectory(src/flightsim)
