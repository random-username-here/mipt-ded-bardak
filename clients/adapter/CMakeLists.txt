cmake_minimum_required(VERSION 3.10)
project(adapter_client)

add_executable(adapter_client
    main.cpp
    proxy/src/main.cpp
    proxy/src/signals.cpp
)

target_link_libraries(adapter_client PRIVATE client-core)
