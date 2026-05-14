#include <cstdio>
#include <cstdlib>
#include <ext/stdio_filebuf.h>
#include <istream>
#include <ostream>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <iostream>
#include <unistd.h>

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

    int pipe2ch[2];
    int pipe4ch[2];

    if (
        pipe (pipe2ch) || 
        pipe (pipe4ch)
    )
    {
        perror ("could not open pipe");
        abort ();
    }

    pid_t pid = fork ();
    if (  pid == -1)
    {
        perror ("fork failed");

        close (pipe2ch[0]);
        close (pipe2ch[1]);
        close (pipe4ch[0]);
        close (pipe4ch[1]);

        abort ();
    }

    if (pid == 0)   // child
    {
        if (
            dup2 (
                pipe2ch[0],
                STDIN_FILENO
            ) == -1
        )
        {
            perror ("dup2");
            
            close (pipe2ch[0]);
            close (pipe2ch[1]);
            close (pipe4ch[0]);
            close (pipe4ch[1]);

            abort ();
        }
        close (pipe2ch[1]);

        if (
            dup2 (
                pipe4ch[1],
                STDOUT_FILENO
            ) == -1
        )
        {
            perror ("dup2");

            close (pipe2ch[0]);
            close (pipe4ch[0]);
            close (pipe4ch[1]);

            abort ();
        }
        close (pipe4ch[0]);

        execlp (
            argv[1],
            argv[1],
            NULL
        );
        perror ("execlp failed");

        close (pipe2ch[0]);
        close (pipe4ch[1]);

        abort ();
    }

    // parent
    close (pipe2ch[0]);
    close (pipe4ch[1]);

    int fd2ch = pipe2ch[1];
    int fd4ch = pipe4ch[0];

    __gnu_cxx::stdio_filebuf<char> outbuf (fd2ch, std::ios::out);
    __gnu_cxx::stdio_filebuf<char> inbuf  (fd4ch, std::ios::in );
    
    std::ostream s2ch (&outbuf );
    std::istream s4ch (&inbuf);
}
