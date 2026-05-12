project(msva)

find_package(PkgConfig)
pkg_check_modules(inih REQUIRED IMPORTED_TARGET GLOBAL inih)

add_library(msva-internals INTERFACE)
target_link_libraries(msva-internals INTERFACE modlib modlib-ifc binmsg libpan)
target_include_directories(msva-internals INTERFACE src/)

pan_header(../../specs/proto/srv.pan src/srv_proto.hpp)
add_executable(msva src/main.cpp src/srv.cpp src/srv_proto.hpp)
target_link_libraries(msva PRIVATE PkgConfig::inih modlib modlib-ifc binmsg libpan_cxx_macros libpan)

