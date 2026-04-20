project(modlib-ifc)

add_library(modlib-ifc INTERFACE)
target_include_directories(modlib-ifc INTERFACE inc/)
target_link_libraries(modlib-ifc INTERFACE modlib binmsg)
target_compile_features(modlib-ifc INTERFACE cxx_std_17)
