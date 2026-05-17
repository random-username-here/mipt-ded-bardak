#pragma once

#include <cstddef>
#include <ext/stdio_filebuf.h>
#include <istream>
#include <ostream>
#include <string_view>
#include <sys/types.h>


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
    int terminate ();
    int  stop      ();

    void fd (int descr[2]) const;

    ssize_t write (const void* buf, size_t size);
    ssize_t read  (      void* buf, size_t size);
    void   flush ();

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

    int m_fd2ch;
    int m_fd4ch;

private:
    Status m_status;
    pid_t  m_child;

    __gnu_cxx::stdio_filebuf<char> m_outbuf;
    __gnu_cxx::stdio_filebuf<char> m_inbuf;

    std::istream m_s4ch;
    std::ostream m_s2ch;
};