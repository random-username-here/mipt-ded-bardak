project(random_client)

add_executable(random_client 
    src/bot.c
    src/net.c
    src/protocol.c
    main.c
)

target_include_directories(random_client PRIVATE inc/)