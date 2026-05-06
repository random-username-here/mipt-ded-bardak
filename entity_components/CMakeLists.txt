project(entity-components-ifc)

add_library(entity-components-ifc STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/EntityBase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Armor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Attack.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Group.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Health.cpp
)

target_include_directories(entity-components-ifc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_link_libraries(entity-components-ifc PUBLIC modlib-ifc binmsg)
target_compile_features(entity-components-ifc PUBLIC cxx_std_17)

