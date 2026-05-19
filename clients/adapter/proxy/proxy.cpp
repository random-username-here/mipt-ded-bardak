#include "proxy.hpp"
#include <cerrno>
#include <cstddef>
#include <ostream>
#include <stdexcept>
#include <string_view>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>
#include <csignal>
#include <cstdio>
#include <sys/wait.h>


Proxy::Proxy (
    const std::string_view filename
)
: m_s4ch(&m_inbuf)
, m_s2ch(&m_outbuf)
{
    int pipe2ch[2];
    int pipe4ch[2];

    if (
        pipe (pipe2ch) ||
        pipe (pipe4ch)
    )
    {
        perror ("pipe() failed");
        throw std::system_error (
            errno,
            std::system_category (),
            "pipe() failed"
        );
    }

    m_child = fork ();
    if (m_child == -1)
    {
        perror ("fork() failed");

        close (pipe2ch[0]);
        close (pipe2ch[1]);
        close (pipe4ch[0]);
        close (pipe4ch[1]);

        throw std::system_error (
            errno,
            std::system_category (),
            "fork() failed"
        );
    }

    if (m_child == 0)  // child
    {
        if (
            dup2 (
                pipe2ch[0],
                STDIN_FILENO
            ) == -1
        )
        {
            perror ("dup2() failed");

            close (pipe2ch[0]);
            close (pipe2ch[1]);
            close (pipe4ch[0]);
            close (pipe4ch[1]);

            throw std::system_error (
                errno,
                std::system_category (),
                "dup2() failed"
            );
        }
        close (pipe2ch[1]);

        if (
            dup2 (
                pipe4ch[1],
                STDOUT_FILENO
            ) == -1
        )
        {
            perror ("dup2() failed");

            close (pipe2ch[0]);
            close (pipe4ch[0]);
            close (pipe4ch[1]);

            throw std::system_error (
                errno,
                std::system_category (),
                "dup2() failed"
            );
        }
        close (pipe4ch[0]);

        execlp (
            filename.data (),
            filename.data (),
            NULL
        );
        perror ("execlp() failed");

        close (pipe2ch[0]);
        close (pipe4ch[1]);

        throw std::system_error (
            errno,
            std::system_category (),
            "execlp() failed"
        );
    }

    // parent

    close (pipe2ch[0]);
    close (pipe4ch[1]);

    m_fd2ch = pipe2ch[1];
    m_fd4ch = pipe4ch[0];

    m_outbuf = __gnu_cxx::stdio_filebuf<char> (
        m_fd2ch,
        std::ios::out
    );
    m_inbuf  = __gnu_cxx::stdio_filebuf<char> (
        m_fd4ch,
        std::ios::in
    );

    m_status = Status::RUNNING;
}

Proxy::~Proxy ()
{
    if (m_status != Status::STOPPED)
    {
        stop ();
    }

    close (m_fd2ch);
    close (m_fd4ch);
}

void
Proxy::fd (
    int descr[2]
) const
{
    if (descr == nullptr)
    {
        throw std::invalid_argument (
            "provided nullptr"
        );
    }

    descr[0] = m_fd4ch;
    descr[1] = m_fd2ch;
}

pid_t
Proxy::pid () const
{
    return m_child;
}

void
Proxy::flush ()
{
    m_s2ch.flush ();
}

ssize_t
Proxy::write (
    const void* buf,
    size_t size
)
{
    m_s2ch.flush ();

    return ::write(
        m_fd2ch,
        buf,
        size
    );
}

ssize_t
Proxy::read (
    void* buf,
    size_t size
)
{
    return ::read(
        m_fd4ch,
        buf,
        size
    );
}

bool
Proxy::freeze () noexcept
{
    if (
        m_status == Status::RUNNING &&
        send (
            SIGSTOP
        )
    )
    {
        m_status = Status::FREEZED;
        return true;
    }

    return false;
}
bool
Proxy::run () noexcept
{
    if (
        m_status == Status::FREEZED &&
        send (
            SIGCONT
        )
    )
    {
        m_status = Status::RUNNING;
        return true;
    }

    return false;
}

int
Proxy::stop ()
{
    if (
        kill (
            m_child,
            SIGKILL
        ) != 0
    )
    {
        throw std::system_error (
            errno,
            std::system_category (),
            "kill() failed"
        );
    }

    int status;
    waitpid (
        m_child,
        &status,
        0
    );

    m_status = Status::STOPPED;
    return status;
}

int
Proxy::terminate ()
{
    if (
        kill (
            m_child,
            SIGTERM
        ) != 0
    )
    {
        throw std::system_error (
            errno,
            std::system_category (),
            "kill() failed"
        );
    }

    int status;
    waitpid (
        m_child,
        &status,
        0
    );

    m_status = Status::STOPPED;
    return status;
}

bool
Proxy::send (
    int sig
) const
{
    if (kill (
            m_child,
            sig
        )
    )
    {
        perror (
            "kill() failed"
        );
        return false;
    }

    return true;
}

Proxy::Status
Proxy::status () const noexcept
{
    return m_status;
}
