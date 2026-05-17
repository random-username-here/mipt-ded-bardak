#pragma once

#include <ext/stdio_filebuf.h>
#include <istream>
#include <ostream>
#include <string_view>


class Proxy
{
public:
    enum class Status
    {
        RUNNING,
        FREEZED,
        WAITING,
        STOPPED
    };


    Proxy (const std::string_view filename);
    ~Proxy ();

    Proxy            (const Proxy& ) = delete;
    Proxy& operator= (const Proxy& ) = delete;


    Status status () const noexcept;

    bool freeze    () noexcept;
    bool run       () noexcept;
    bool terminate () noexcept;
    int  stop      () noexcept;



    template <typename T>
    Proxy& 
    operator<< (
        const T& value
    )
    {
        m_s2ch << value;
        return *this;
    }

    Proxy& 
    operator<< (
        std::ostream& (*pf)(std::ostream&)
    )
    {
        m_s2ch << pf;
        return *this;
    }

    template <typename T>
    Proxy&
    operator>> (
        T& value
    )
    {
        m_s4ch >> value;
        return *this;
    }



protected:
    bool send (int sig) const;

private:
    

    Status m_status;
    pid_t  m_child;

    int m_fd2ch;
    int m_fd4ch;

    __gnu_cxx::stdio_filebuf<char> m_outbuf;
    __gnu_cxx::stdio_filebuf<char> m_inbuf;

    std::istream m_s4ch;
    std::ostream m_s2ch;
};