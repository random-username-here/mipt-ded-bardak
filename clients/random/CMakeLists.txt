cmake_minimum_required(VERSION 3.16)
project(random_client C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(random_client
    src/bot.c
    src/net.c
    src/protocol.c
    src/main.c
)

target_include_directories(random_client PRIVATE inc)

target_compile_options(random_client PRIVATE -Wall -Wextra -Wpedantic)
