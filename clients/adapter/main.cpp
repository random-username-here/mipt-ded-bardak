#include <sys/socket.h>
#include <sys/types.h>
#include <iostream>
#include <unistd.h>
#include "client-core/base/client_base.hpp"


int
main (
    int    argc,
    char** argv
)
{
    if (argc < 3)
    {
        std::cerr << "usage: \"" << argv[0] << " *path2script* *settings.ini*" << std::endl;
    }

    int pipeIn [2];
    int pipeOut[2];

}