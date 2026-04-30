project(asset_manager)

add_library(asset_manager MODULE main.cpp)
target_link_libraries(asset_manager PRIVATE modlib modlib-ifc)

set_target_properties(asset_manager PROPERTIES OUTPUT_NAME "nellor.bardak.asset_manager")
set_target_properties(asset_manager PROPERTIES PREFIX "")
set_target_properties(asset_manager PROPERTIES SUFFIX ".mod")

