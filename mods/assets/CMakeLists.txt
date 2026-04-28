project(asset_manager)

add_library(asset_manager MODULE main.cpp asset_proto.hpp)
target_link_libraries(asset_manager PRIVATE modlib modlib-ifc binmsg libpan_cxx_macros)

set_target_properties(asset_manager PROPERTIES OUTPUT_NAME "isd.bardak.asset_manager")
set_target_properties(asset_manager PROPERTIES PREFIX "")
set_target_properties(asset_manager PROPERTIES SUFFIX ".mod")

