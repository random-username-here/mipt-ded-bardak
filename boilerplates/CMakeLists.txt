project(boilerplates)

add_library(boilerplates INTERFACE)
target_include_directories(boilerplates INTERFACE inc/)
target_compile_features(boilerplates INTERFACE cxx_std_17)
