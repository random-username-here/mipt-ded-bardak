project(msva)

find_package(PkgConfig)
pkg_check_modules(inih REQUIRED IMPORTED_TARGET GLOBAL inih)

pan_header(src/srv_proto.pan src/srv_proto.hpp)

add_library(msva.server SHARED)
target_link_libraries(msva.server PUBLIC modlib libpan aixlog libpan_cxx_macros)
target_include_directories(msva.server PUBLIC inc/)
target_sources(msva.server PRIVATE src/server.cpp src/srv_proto.hpp src/common.cpp)

add_library(msva.client STATIC)
target_link_libraries(msva.client PUBLIC libpan libpan_cxx_macros aixlog)
target_include_directories(msva.client PUBLIC inc/)
target_sources(msva.client PRIVATE src/client.cpp src/srv_proto.hpp src/common.cpp)

add_executable(msva src/main.cpp)
target_link_libraries(msva PRIVATE PkgConfig::inih msva.server)


