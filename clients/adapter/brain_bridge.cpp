#include "brain_bridge.hpp"
#include "brain_protocol.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#ifdef ERROR
#undef ERROR
#endif
#else
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace
{

#ifdef _WIN32
void ensureWinsock()
{
    static bool ready = false;
    if (!ready) {
        WSADATA wsa{};
        WSAStartup(MAKEWORD(2, 2), &wsa);
        ready = true;
    }
}

void closeFd(SOCKET fd)
{
    closesocket(fd);
}

bool isWouldBlock()
{
    const int err = WSAGetLastError();
    return err == WSAEWOULDBLOCK || err == WSAEINTR;
}
#else
void ensureWinsock() {}

void closeFd(int fd)
{
    close(fd);
}

bool isWouldBlock()
{
    return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR;
}
#endif

} // namespace

BrainBridge::BrainBridge(std::string_view host, std::string_view port)
    : m_host(host)
    , m_port(port)
{}

BrainBridge::~BrainBridge()
{
    close();
}

void BrainBridge::close()
{
    if (m_client_fd >= 0) {
#ifdef _WIN32
        shutdown(m_client_fd, SD_BOTH);
#else
        shutdown(m_client_fd, SHUT_RDWR);
#endif
        closeFd(m_client_fd);
        m_client_fd = -1;
    }
    if (m_listen_fd >= 0) {
        closeFd(m_listen_fd);
        m_listen_fd = -1;
    }
}

bool BrainBridge::listen()
{
    ensureWinsock();

    addrinfo hints{};
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;

    addrinfo *res = nullptr;
    const int gai = getaddrinfo(m_host.c_str(), m_port.c_str(), &hints, &res);
    if (gai != 0) {
        std::cerr << "brain bridge getaddrinfo: " << gai_strerror(gai) << '\n';
        return false;
    }

    int fd = -1;
    for (addrinfo *it = res; it != nullptr; it = it->ai_next) {
        fd = static_cast<int>(socket(it->ai_family, it->ai_socktype, it->ai_protocol));
        if (fd < 0) {
            continue;
        }

        const int yes = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&yes),
                   sizeof(yes));

        if (bind(fd, it->ai_addr, static_cast<int>(it->ai_addrlen)) == 0 && ::listen(fd, 1) == 0) {
            break;
        }

        closeFd(fd);
        fd = -1;
    }

    freeaddrinfo(res);

    if (fd < 0) {
        std::cerr << "brain bridge listen failed\n";
        return false;
    }

    m_listen_fd = fd;
    return true;
}

bool BrainBridge::launchScript(std::string_view executable)
{
#ifdef _WIN32
    std::string cmd(executable);
    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    if (!CreateProcessA(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si,
                        &pi)) {
        std::cerr << "CreateProcess failed: " << GetLastError() << '\n';
        return false;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
#else
    const pid_t child = fork();
    if (child < 0) {
        perror("fork");
        return false;
    }

    if (child == 0) {
        execlp(executable.data(), executable.data(), static_cast<char *>(nullptr));
        perror("execlp");
        _exit(127);
    }

    return true;
#endif
}

bool BrainBridge::acceptScript(int timeout_ms)
{
    return waitForScript(timeout_ms);
}

bool BrainBridge::waitForScript(int timeout_ms)
{
    if (m_listen_fd < 0) {
        return false;
    }

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(m_listen_fd, &rfds);

    timeval tv{};
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    const int sel = select(m_listen_fd + 1, &rfds, nullptr, nullptr, &tv);
    if (sel <= 0) {
        std::cerr << "brain bridge accept timeout\n";
        return false;
    }

    const int client = static_cast<int>(accept(m_listen_fd, nullptr, nullptr));
    if (client < 0) {
        perror("accept");
        return false;
    }

    m_client_fd = client;
    return true;
}

bool BrainBridge::waitReadable(int timeout_ms)
{
    if (m_client_fd < 0) {
        return false;
    }

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(m_client_fd, &rfds);

    timeval tv{};
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    const int sel = select(m_client_fd + 1, &rfds, nullptr, nullptr, &tv);
    return sel > 0 && FD_ISSET(m_client_fd, &rfds);
}

bool BrainBridge::waitWritable(int timeout_ms)
{
    if (m_client_fd < 0) {
        return false;
    }

    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(m_client_fd, &wfds);

    timeval tv{};
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    const int sel = select(m_client_fd + 1, nullptr, &wfds, nullptr, &tv);
    return sel > 0 && FD_ISSET(m_client_fd, &wfds);
}

bool BrainBridge::sendInt(int32_t value)
{
    if (m_client_fd < 0) {
        return false;
    }

    char line[72];
    const int len = std::snprintf(line, sizeof(line), "%d\n", value);
    if (len <= 0) {
        return false;
    }

    std::size_t sent = 0;
    while (sent < static_cast<std::size_t>(len)) {
        if (!waitWritable(5000)) {
            return false;
        }

        const int n = send(m_client_fd, line + sent, static_cast<int>(len - sent), 0);
        if (n < 0) {
            if (isWouldBlock()) {
                continue;
            }
            return false;
        }
        if (n == 0) {
            return false;
        }
        sent += static_cast<std::size_t>(n);
    }

    return true;
}

bool BrainBridge::recvInt(int32_t &value, int timeout_ms)
{
    if (m_client_fd < 0) {
        return false;
    }

    char buf[96];
    int total = 0;

    while (total < static_cast<int>(sizeof(buf)) - 1) {
        if (!waitReadable(timeout_ms)) {
            return false;
        }

        char c = 0;
        const int n = recv(m_client_fd, &c, 1, 0);
        if (n < 0) {
            if (isWouldBlock()) {
                continue;
            }
            return false;
        }
        if (n == 0) {
            return false;
        }

        if (c == '\n' || c == '\r') {
            break;
        }

        buf[total++] = c;
    }

    buf[total] = '\0';
    value = static_cast<int32_t>(std::strtol(buf, nullptr, 10));
    return true;
}

bool BrainBridge::sendTick()
{
    return sendInt(brain_proto::kEvtTick);
}

bool BrainBridge::sendHp(int32_t hp)
{
    return sendInt(brain_proto::kEvtHp) && sendInt(hp);
}

bool BrainBridge::sendAt(int32_t x, int32_t y)
{
    return sendInt(brain_proto::kEvtAt) && sendInt(x) && sendInt(y);
}

bool BrainBridge::sendRoot(int32_t x, int32_t y, uint32_t id)
{
    return sendInt(brain_proto::kEvtRoot) && sendInt(x) && sendInt(y) &&
           sendInt(static_cast<int32_t>(id));
}

bool BrainBridge::sendEnemy(int32_t x, int32_t y, uint32_t id)
{
    return sendInt(brain_proto::kEvtEnemy) && sendInt(x) && sendInt(y) &&
           sendInt(static_cast<int32_t>(id));
}

bool BrainBridge::sendWall(int32_t x, int32_t y)
{
    return sendInt(brain_proto::kEvtWall) && sendInt(x) && sendInt(y);
}

bool BrainBridge::sendAbilitySlash()
{
    return sendInt(brain_proto::kEvtAbilitySlash);
}

bool BrainBridge::queryAction(Action &out, int timeout_ms)
{
    int32_t kind = brain_proto::kActNone;
    if (!recvInt(kind, timeout_ms)) {
        return false;
    }

    out.kind = kind;
    out.dx = 0;
    out.dy = 0;
    out.target = 0;

    if (kind == brain_proto::kActMove) {
        if (!recvInt(out.dx, timeout_ms) || !recvInt(out.dy, timeout_ms)) {
            return false;
        }
    } else if (kind == brain_proto::kActUse) {
        int32_t target = 0;
        if (!recvInt(target, timeout_ms)) {
            return false;
        }
        out.target = static_cast<uint32_t>(target);
    }

    return true;
}
