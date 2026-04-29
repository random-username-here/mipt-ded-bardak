project(client_utils)

add_library(utils
	tcp/tcp_connection.cpp
	tcp/tcp_connection.hpp
	base/client_base.cpp
	base/client_base.hpp
)

target_include_directories(utils PUBLIC
	${CMAKE_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(utils PUBLIC generated_proto libpan PkgConfig::inih)
