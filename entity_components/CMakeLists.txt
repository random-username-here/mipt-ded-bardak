project(entity-components-ifc)

add_library(entity-components-ifc STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/EC/Armor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/IC/Drop.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/EC/Group.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/EC/Health.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/IC/Durability.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/EC/Movable.cpp
)

target_include_directories(entity-components-ifc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_compile_features(entity-components-ifc PUBLIC cxx_std_17)
target_link_libraries(entity-components-ifc PUBLIC modlib-ifc binmsg)

