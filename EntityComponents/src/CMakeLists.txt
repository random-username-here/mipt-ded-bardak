project(EntityComponents)

add_library(EntityComponents MODULE 
    EntityBase.cpp
    Group.cpp
    Health.cpp
    Armor.cpp
)

target_link_libraries(EntityComponents PRIVATE EntityComponents-ifc)

set_target_properties(EntityComponents PROPERTIES OUTPUT_NAME "dodo6be.EC")
set_target_properties(EntityComponents PROPERTIES PREFIX "")
set_target_properties(EntityComponents PROPERTIES SUFFIX ".mod")

