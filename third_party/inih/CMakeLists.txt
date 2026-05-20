add_library(inih STATIC ini.c)
target_include_directories(inih PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
