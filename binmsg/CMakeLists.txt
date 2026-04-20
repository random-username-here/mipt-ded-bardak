project(binmsg)

add_library(binmsg INTERFACE)
target_include_directories(binmsg INTERFACE inc/)
target_compile_features(binmsg INTERFACE cxx_std_17)
