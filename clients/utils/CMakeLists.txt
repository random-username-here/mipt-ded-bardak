project(client_scripts_utils)

add_library(forini
	tcp_connection.cpp
	tcp_connection.hpp
	client_base.cpp
	client_base.hpp
)
target_link_libraries(forini libpan_cxx_macros libpan PkgConfig::inih)
