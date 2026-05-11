project(client-core)

add_library(client-core
	tcp/tcp_connection.cpp
	tcp/tcp_connection.hpp
	base/client_base.cpp
	base/client_base.hpp
)

target_include_directories(client-core PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/proto
)

target_include_directories(client-core PUBLIC
	${CMAKE_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(client-core PUBLIC libpan PkgConfig::inih)
