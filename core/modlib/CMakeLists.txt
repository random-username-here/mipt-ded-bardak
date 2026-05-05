project(modlib)

add_library(modlib STATIC)
target_include_directories(modlib PUBLIC inc/)
#target_link_libraries(modlib PUBLIC misclib)
target_sources(modlib PRIVATE src/modlib_manager.cpp)
