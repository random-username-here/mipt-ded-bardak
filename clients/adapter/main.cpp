#include <iostream>
#include "client-core/tcp/tcp_connection.hpp"
#include "proxy/inc.hpp"

int main (
    int    argc,
    char** argv
)
{
    if (argc < 3)
    {
        std::cerr << "usage: \"" << argv[0] << " *path2script* *settings.ini*" << std::endl;
        
        return EXIT_FAILURE;
    }

    Proxy script (argv[1]);

    
}
