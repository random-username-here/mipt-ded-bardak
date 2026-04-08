project(textmsg_server)

add_library(textmsg_server INTERFACE)
target_include_directories(textmsg_server INTERFACE .)
target_compile_features(textmsg_server INTERFACE cxx_std_20)
