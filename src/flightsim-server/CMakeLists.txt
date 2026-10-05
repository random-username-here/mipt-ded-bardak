add_library(flightsim.server SHARED)
target_link_libraries(flightsim.server PUBLIC aixlog msva.server modlib syncv.server)
target_include_directories(flightsim.server PUBLIC src/)
target_sources(flightsim.server PRIVATE src/server.cpp)
