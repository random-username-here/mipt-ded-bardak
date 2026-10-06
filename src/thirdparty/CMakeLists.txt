add_library(aixlog INTERFACE)
target_include_directories(aixlog INTERFACE aixlog/)

add_library(hmmath INTERFACE)
target_include_directories(hmmath INTERFACE hmmath/)


find_package(imgui CONFIG REQUIRED)

add_library(rlimgui STATIC rimgui/rlImGui.cpp)
target_include_directories(rlimgui PUBLIC rimgui/)
target_link_libraries(rlimgui PUBLIC imgui::imgui)
