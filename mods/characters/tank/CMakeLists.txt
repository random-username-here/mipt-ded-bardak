project(tank)

include("${CMAKE_CURRENT_SOURCE_DIR}/proto/CharacterPanProto.cmake")

add_library(tank MODULE main.cpp "${TANK_PROTO_FILE}")

add_custom_target(generate_tank_mod_proto DEPENDS "${TANK_PROTO_FILE}")
add_dependencies(tank generate_tank_mod_proto)

target_include_directories(tank PRIVATE 
    "${TANK_PROTO_GEN_DIR}" 
    ${CMAKE_CURRENT_SOURCE_DIR}/inc 
    .. 
    ../common
)

target_link_libraries(tank PRIVATE modlib modlib-ifc map-core binmsg libpan_cxx_macros)
target_compile_definitions(tank PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
set_target_properties(tank PROPERTIES OUTPUT_NAME "ida.bardak.uctl.tank")
set_target_properties(tank PROPERTIES PREFIX "")
set_target_properties(tank PROPERTIES SUFFIX ".mod")
