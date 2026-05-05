project(EntityComponents)

add_library(map MODULE 
    EntityBase.cpp
    Group.cpp
    Health.cpp
)

target_link_libraries(map PRIVATE EntityComponents-ifc)

set_target_properties(map PROPERTIES OUTPUT_NAME "ida.bardak.map")
set_target_properties(map PROPERTIES PREFIX "")
set_target_properties(map PROPERTIES SUFFIX ".mod")

