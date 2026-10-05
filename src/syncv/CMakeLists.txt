pan_header(inc/syncv/proto.pan inc/syncv/proto.hpp)

add_library(syncv.client STATIC)
target_link_libraries(syncv.client PUBLIC aixlog msva.client)
target_include_directories(syncv.client PUBLIC inc/)
target_sources(syncv.client PRIVATE src/client.cpp inc/syncv/proto.hpp)

add_library(syncv.server SHARED)
target_link_libraries(syncv.server PUBLIC aixlog msva.server modlib)
target_include_directories(syncv.server PUBLIC inc/)
target_sources(syncv.server PRIVATE src/server.cpp inc/syncv/proto.hpp)
