#include "../inc.hpp"
#include <csignal>
#include <cstdio>
#include <sys/wait.h>


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
Proxy::stop () noexcept
{
    kill (
        m_child,
        SIGKILL
    );

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
Proxy::terminate () noexcept
{
    if (
        send (
            SIGTERM
        )
    )
    {
        m_status = Status::STOPPED;
        return true;
    }

    return false;
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