cmake_minimum_required(VERSION 3.10)
project(bardak)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for IDE-s
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # for .so-s

add_subdirectory(std/textmsg_server)
