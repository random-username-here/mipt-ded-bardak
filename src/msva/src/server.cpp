#include "MSVA.hpp"
#include "binmsg.hpp"
#include "libpan.h"
#include "modlib_manager.hpp"
#include "srv_proto.hpp"
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include "aixlog.hpp"

#define BUF_SIZE 1024

namespace msva {

MsvaUser::MsvaUser(MsvaServer *s, int tcp, int udp, size_t id, sockaddr_in addr)
    :m_tcp(tcp), m_udp(udp), m_server(s), m_id(id), m_addr(addr) {
    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, (const void*) &addr.sin_addr, buf, INET_ADDRSTRLEN);
    std::ostringstream oss;
    oss << "msva/client/" << buf << ":" << ntohs(addr.sin_port);
    m_tag = oss.str();
}

void MsvaUser::send(bmsg::RawMessage m) {
    if (m_disconnecting || m_tcp < 0) {
        return;
    }

    m_server->m_pan->userptr = m_tag.data();
    pan_binDump_short(m_server->m_pan, PAN_SERVER, m.data().data(), m.data().size());
    m_server->m_pan->userptr = nullptr;

    if (m.header()->flags.has(bmsg::USE_UDP)) {
        // TODO: buffer stuff
        uint32_t id = m_id;
        iovec iov[2] = {
            { .iov_base = (void*) &id, .iov_len = 4 },
            { .iov_base = (void*) m.data().data(), .iov_len = m.data().size() }
        };
        msghdr mh = { 
            .msg_name = &m_addr,
            .msg_namelen = sizeof(m_addr),
            .msg_iov = iov,
            .msg_iovlen = 2
        };
        ssize_t err = ::sendmsg(m_udp, &mh, MSG_NOSIGNAL);
        if (err < 0)
            LOG(ERROR, m_tag) << "UDP sendmsg() failed: " << strerror(errno) << '\n';
    } else {
        ssize_t err = ::send(m_tcp, m.data().data(), m.data().size(), MSG_NOSIGNAL);
        if (err < 0)
            LOG(ERROR, m_tag) << "TCP send() failed: " << strerror(errno) << '\n';
    } 
}

static uint64_t l_ch64u(std::string_view s) {
    bmsg::Char64 ch(s);
    return ch.as_u64;
}

bool MsvaServer::registerPrefix(std::string_view pref, MsvaModule *mod) {
    if (pref == "srv") return false;
    uint64_t u = l_ch64u(pref);
    if (m_prefMapping.count(u)) return false;
    m_prefMapping[u] = mod;
    LOG(INFO, "msva/server") << "Plugin " << mod->id() << " now responsible for pref " << pref << "\n";
    return true;
}

void MsvaServer::listenPrefix(std::string_view pref, MsvaModule *mod) {
    uint64_t u = l_ch64u(pref);
    m_listeners[u].push_back(mod);
    LOG(INFO, "msva/server") << "Plugin " << mod->id() << " now listening to pref " << pref << "\n";
}

void MsvaServer::listenAll(MsvaModule *mod) {
    m_listenersForAll.push_back(mod);
    LOG(INFO, "msva/server") << "Plugin " << mod->id() << " now listening to everything\n";
}

void MsvaServer::m_processMessage(MsvaUser *cl, bmsg::RawMessage msg) {
    
    m_pan->userptr = cl->m_tag.data();
    pan_binDump_short(m_pan, PAN_CLIENT, msg.data().data(), msg.data().size());
    m_pan->userptr = nullptr;

    assert(msg.isCorrect());
    uint64_t pref = msg.header()->pref.as_u64;

    if (msg.header()->pref == "srv")
        m_srvMessage(cl, msg);
    else if (m_prefMapping.count(pref))
        m_prefMapping.at(pref)->onMessage(cl, msg);

    if (m_listeners.count(pref))
        for (auto i : m_listeners.at(pref))
            i->onMessage(cl, msg);

    for (auto i : m_listenersForAll)
        i->onMessage(cl, msg);
}

void MsvaServer::m_srvMessage(MsvaUser *client, bmsg::RawMessage msg) {
    assert(msg.isCorrect());
    if (msg.header()->type == "tLocal") {
        // client time sync
        auto req = bmsg::CL_srv_tLocal::decode(msg);
        if (req)
            client->send(bmsg::SV_srv_tCorrect { req->client_ns, nsTime() }, bmsg::USE_UDP | bmsg::NO_BUF);
    } else {
        // drop
    }
    return;
}

void MsvaServer::setConfigValue(std::string_view key, std::string_view value) {
    m_config[std::string(key)] = std::string(value);
}

std::optional<std::string> MsvaServer::configValue(std::string_view key) const {
    auto it = m_config.find(std::string(key));
    if (it == m_config.end()) {
        return std::nullopt;
    }
    return it->second;
}

void MsvaServer::disconnectClient(MsvaUser *client) {
    if (client == nullptr) {
        return;
    }

    disconnectClient(client->id());
}

void MsvaServer::disconnectClient(size_t clientId) {
    auto it = m_clients.find(clientId);
    if (it == m_clients.end()) {
        return;
    }

    m_disconnect(it->second.get());
}

void MsvaServer::m_disconnect(MsvaUser *cl) {
    if (cl == nullptr || cl->m_disconnecting) {
        return;
    }

    cl->m_disconnecting = true;
    LOG(NOTICE, cl->m_tag) << "Client disconnected\n";

    if (cl->m_tcp >= 0) {
        epoll_ctl(m_epollFd, EPOLL_CTL_DEL, cl->m_tcp, NULL);
        close(cl->m_tcp);
        cl->m_tcp = -1;
    }

    for (auto i : m_plugins) {
        i->onDisconnect(cl);
    }

    m_clients.erase(cl->m_id);
}

void MsvaServer::m_onConnect(MsvaUser * cl) {
    cl->send(bmsg::SV_srv_name {m_name});
    cl->send(bmsg::SV_srv_id {(uint32_t) cl->m_id});
    for (auto [pref, mod] : m_prefMapping)
        cl->send(bmsg::SV_srv_hasPref { pref });
    cl->send(bmsg::SV_srv_hasPref { "" });

    for (auto i : m_plugins)
        i->onConnect(cl);
}

MsvaServer::MsvaServer(ModManager *mm, PAN *pan) {
    m_pan = pan;
    m_mm = mm;
    m_port = 3000;
    m_tickTime = 13000000;
    m_name = "Unnamed server";
}

void MsvaServer::initMods() {
    m_plugins = m_mm->allOfType<MsvaModule>();
    for (auto mod : m_plugins) {
        LOG(INFO, "msva/server") << "Adding server module " << mod->id() << "\n";
        mod->setServer(this);
        mod->onSetup(this);
    }
}

void MsvaServer::m_addToEpoll(void *ptr, int fd, uint32_t flags) {
    LOG(TRACE, "msva/server") << "Adding " << fd << " with ptr " << ptr << " to epoll\n";
    epoll_event e;
    e.events = flags;
    e.data.ptr = ptr;
    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &e) == -1) {
        LOG(FATAL, "msva/server") << "Failed to add fd to epoll: " << strerror(errno) << "\n";
        abort();
    }
}

void MsvaServer::m_incoming(MsvaUser *cl, std::string_view data) {
    cl->m_tcp_inBuffer += data;

    while (true) {
        bmsg::RawMessage m(cl->m_tcp_inBuffer);
        if (!m.header()) {
            break;
        }

        const size_t len = sizeof(bmsg::Header) + m.header()->len;
        if (len > cl->m_tcp_inBuffer.size()) {
            break;
        }

        m_processMessage(cl, bmsg::RawMessage(cl->m_tcp_inBuffer.substr(0, len)));
        cl->m_tcp_inBuffer = cl->m_tcp_inBuffer.substr(len);

        if (m_clients.find(cl->m_id) == m_clients.end()) {
            break;
        }
    }
}

void MsvaServer::mainloop() {

    LOG(NOTICE, "msva/server") << "Starting server `" << m_name << "` @ port " << m_port << "\n";
    LOG(INFO, "msva/server") << "Update time is " << m_tickTime << "\n";

#define ERROR_IF(cond, msg)\
    do { if (cond) {\
        LOG(FATAL, "msva/server") << msg << ": " << strerror(errno) << "\n";\
        return;\
    } } while (0)

    m_tcpSockFd = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_IF(m_tcpSockFd < 0, "Failed to open tcp socket");

    m_udpSockFd = socket(AF_INET, SOCK_DGRAM, 0);
    ERROR_IF(m_tcpSockFd < 0, "Failed to open udp socket");
    
    LOG(TRACE, "msva/server") << "TCP socket is " << m_tcpSockFd << ", udp socket is " << m_udpSockFd << '\n';

    sockaddr_in sin = {};
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(m_port);

    int en = 1, err;

    err = setsockopt(m_tcpSockFd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en));
    ERROR_IF(err < 0, "Failed to set SO_REUSEADDR on tcp socket");
    err = setsockopt(m_udpSockFd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en));
    ERROR_IF(err < 0, "Failed to set SO_REUSEADDR on udp socket");

    err = bind(m_tcpSockFd, (sockaddr*) &sin, sizeof(sin));
    ERROR_IF(err < 0, "Failed to bind TCP socket");
    err = bind(m_udpSockFd, (sockaddr*) &sin, sizeof(sin));
    ERROR_IF(err < 0, "Failed to bind UDP socket");

    err = listen(m_tcpSockFd, 10);
    ERROR_IF(err < 0, "Failed to listen TCP socket");

    void* P_TCP = (void*) (0x1); // values meant for determining tcp/udp epolls
    void* P_UDP = (void*) (0x2);
    
    m_epollFd = epoll_create(1);
    m_addToEpoll(P_TCP, m_tcpSockFd, EPOLLIN | EPOLLOUT | EPOLLET);
    m_addToEpoll(P_UDP, m_udpSockFd, EPOLLIN | EPOLLOUT | EPOLLET);

    char buf[BUF_SIZE] = {0};

    LOG(NOTICE, "msva/server") << "Now listening...\n";

    int64_t nextClock = nsTime();

    while (1) {
        for (auto i : m_plugins)
            i->onIteration();

        int64_t curTime = nsTime(), waitTime = 0;
        if (curTime < nextClock) {
            waitTime = nextClock - curTime;
        } else {
            // todo: ticks
            nextClock += (curTime - nextClock) / m_tickTime * m_tickTime + m_tickTime;
        }

        epoll_event ev;
        int nfds = epoll_wait(m_epollFd, &ev, 1, waitTime / 1e6); // 1s time
        if (nfds < 0) {
            LOG(FATAL, "msva/server") << "epoll_wait() error: " << strerror(errno) << "\n";
            return;
        } if (nfds == 0) {
            continue;
        }

        LOG(TRACE, "msva/server") << "Epoll event: ptr = " << ev.data.ptr 
            << ", ev = " << ev.events << "\n";

        if (ev.data.ptr == P_TCP) {
            // tcp socket
            LOG(TRACE, "msva/server") << "New client connecting to tcp socket...\n";
            size_t id = ++m_lastId;
            sockaddr_in addr;
            socklen_t slen = sizeof(addr);
            int clTcp = accept(m_tcpSockFd, (sockaddr*) &addr, &slen);
            m_clients[id] = std::make_unique<MsvaUser>(MsvaUser(this, clTcp, m_udpSockFd, id, addr));

            LOG(NOTICE, m_clients[id]->m_tag) << "Client connected!\n";
            m_addToEpoll(m_clients[id].get(), clTcp, EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLHUP);
            m_onConnect(m_clients[id].get()); 
        } else if (ev.data.ptr == P_UDP) {
            // udp socket
            // each message has client id prepended
            if (ev.events & EPOLLIN) {
                LOG(TRACE, "msva/server") << "UDP message incoming\n";
                struct iovec iov = { .iov_base = buf, .iov_len = sizeof(buf) };
                struct sockaddr_in addr;
                struct msghdr msg = {
                    .msg_name = &addr, .msg_namelen = sizeof(addr),
                    .msg_iov = &iov, .msg_iovlen = 1
                };
                ssize_t n = recvmsg(m_udpSockFd, &msg, 0);
                //LOG(TRACE, "msva/server") << "Message of size " << n << "\n";
                if (n < 4) 
                    continue;
                uint32_t id;
                memcpy(&id, buf, 4);
                //LOG(TRACE, "msva/server") << "By client ID = " << id << "\n";
                if (!m_clients.count(id)) {
                    //LOG(TRACE, "msva/server") << "Unknown ID, dropping\n";
                    continue;
                }
                auto client = m_clients.at(id).get();
                /*if (memcmp(&addr, &client->m_addr, sizeof(sockaddr_in)) != 0) {
                    LOG(TRACE, "msva/server") << "Bad addr, dropping\n";
                    continue; // TODO: add real authenticity checking
                }*/
                m_processMessage(client, bmsg::RawMessage(std::string_view(buf+4, n-4)));
            }
        } else if (ev.data.ptr) {
            // tcp connection
            if (ev.events & EPOLLIN) {
                LOG(TRACE, "msva/server") << "TCP data incoming\n";
                MsvaUser *cl = (MsvaUser*) ev.data.ptr;
                const size_t clientId = cl->m_id;

                int n = read(cl->m_tcp, buf, sizeof(buf));
                if (n > 0) {
                    m_incoming(cl, std::string_view(buf, n));
                }

                if (m_clients.find(clientId) == m_clients.end()) {
                    continue;
                }
            }
            if (ev.events & (EPOLLRDHUP | EPOLLHUP)) {
                MsvaUser *cl = (MsvaUser*) ev.data.ptr;
                m_disconnect(cl);
            }
        }
    }
}

};
