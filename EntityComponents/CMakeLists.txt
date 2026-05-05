project(EntityComponents-ifc)

add_library(EntityComponents INTERFACE)
target_include_directories(EntityComponents INTERFACE inc/)
target_link_libraries(EntityComponents INTERFACE modlib-ifc binmsg)
target_compile_features(EntityComponents INTERFACE cxx_std_17)
