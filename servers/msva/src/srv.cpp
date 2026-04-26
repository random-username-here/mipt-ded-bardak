#include "./all.hpp"
#include "BmServerModule.hpp"
#include "binmsg.hpp"
#include "libpan.h"
#include "srv_proto.hpp"
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024
#define TICK_TIME 1000

namespace msva {

std::ostream &Client::m_log() {
    static const char *colors[] = {
        ESC_RED, ESC_GRN, ESC_YLW,
        ESC_BLU, ESC_CYN, ESC_MGN
    };

    std::cerr << colors[m_id % 6] << "client " << std::setw(3) << m_id << ESC_GRY << " : " << ESC_RST;
    return std::cerr;
}

void Client::send(bmsg::RawMessage m) {
    m_log();
    pan_binDump_short(m_server->m_pan, PAN_SERVER, m.data().data(), m.data().size());

    ssize_t err = ::send(m_fd, m.data().data(), m.data().size(), MSG_NOSIGNAL);

    if (err < 0) {
        m_log() << "write failed: " << strerror(errno) << '\n';
    }
}

static uint64_t l_ch64u(std::string_view s) {
    bmsg::Char64 ch(s);
    return ch.as_u64;
}

std::ostream &Server::m_log() {
    std::cerr << ESC_GRY << "server ... : " << ESC_RST;
    return std::cerr;
}

bool Server::registerPrefix(std::string_view pref, modlib::BmServerModule *mod) {
    if (pref == "srv") return false;
    uint64_t u = l_ch64u(pref);
    if (m_prefMapping.count(u)) return false;
    m_prefMapping[u] = mod;
    m_log() << "Plugin " << ESC_BLU << mod->id() << ESC_RST << " now responsible for prefix " << ESC_GRN << pref << ESC_RST << '\n';
    return true;
}

void Server::listenPrefix(std::string_view pref, modlib::BmServerModule *mod) {
    uint64_t u = l_ch64u(pref);
    m_listeners[u].push_back(mod);
    m_log() << "Plugin " << ESC_BLU << mod->id() << ESC_RST << " now listening messages for " << ESC_GRN << pref << ESC_RST << '\n';
}

void Server::listenAll(modlib::BmServerModule *mod) {
    m_listenersForAll.push_back(mod);
    m_log() << "Plugin " << ESC_BLU << mod->id() << ESC_RST << " now listening all incoming messages\n";
}

void Server::m_processMessage(Client *cl, bmsg::RawMessage msg) {
    cl->m_log();
    pan_binDump_short(m_pan, PAN_CLIENT, msg.data().data(), msg.data().size());

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

void Server::m_srvMessage(Client *cl, bmsg::RawMessage msg) {
    assert(msg.isCorrect());
    // TODO
    return;
}

void Server::m_onConnect(Client *cl) {
    cl->send(bmsg::SV_srv_name {m_name});
    cl->send(bmsg::SV_srv_id {(uint32_t) cl->m_id});
    cl->send(bmsg::SV_srv_level { "player" });
    for (auto [pref, mod] : m_prefMapping)
        cl->send(bmsg::SV_srv_hasPref { pref });

    for (auto i : m_plugins)
        i->onConnect(cl);
}

Server::Server(ModManager *mm, PAN *pan) {
    m_pan = pan;
    m_mm = mm;
}

void Server::initMods() {
    m_plugins = m_mm->allOfType<modlib::BmServerModule>();
    for (auto mod : m_plugins) {
        m_log() << "Adding BmServerModule " << ESC_BLU << mod->id() << ESC_RST << "\n";
        mod->setServer(this);
        mod->onSetup(this);
    }
    m_tm = m_mm->anyOfType<modlib::Timer>();
    if (!m_tm)
        m_log() << "No timer found, will not do tick()\n";
}

void Server::m_addToEpoll(Client *cl, int fd, uint32_t flags) {
    epoll_event e;
    e.events = flags;
    e.data.ptr = cl;
    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &e) == -1) {
        m_log() << "Failed add fd to epoll: " << strerror(errno) << '\n';
        abort();
    }
}

void Server::m_incoming(Client *cl, std::string_view data) {
    cl->m_partialMsg += data;
maybe_again:
    bmsg::RawMessage m(cl->m_partialMsg);
    if (!m.header()) return;
    size_t len = sizeof(bmsg::Header) + m.header()->len;
    if (len <= cl->m_partialMsg.size()) {
        m_processMessage(cl, bmsg::RawMessage(cl->m_partialMsg.substr(0, len)));
        cl->m_partialMsg = cl->m_partialMsg.substr(len);
        goto maybe_again;
    }
}

static uint64_t l_curMs() {
    struct timespec tm;
    clock_gettime(CLOCK_MONOTONIC, &tm);
    return tm.tv_sec * 1000 + tm.tv_nsec / 1e6;
}

void Server::mainloop() {

    m_log() << "Starting server `" << m_name << "` on port " << m_port << '\n';

    m_sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sockFd < 0) {
        m_log() << "Failed to open socket: " << strerror(errno) << '\n';
        return;
    } 

    sockaddr_in sin = {};
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(m_port);

    int en = 1;
    if (setsockopt(m_sockFd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en)) < 0) {
        m_log() << "Failed to set SO_REUSEADDR on socket: " << strerror(errno) << '\n';
        return;
    }

    if (bind(m_sockFd, (sockaddr*) &sin, sizeof(sin)) < 0) {
        m_log() << "Failed to bind socket: " << strerror(errno) << '\n';
        return;
    }

    if (listen(m_sockFd, 10) < 0) {
        m_log() << "Failed to listen socket: " << strerror(errno) << '\n';
        return;
    }

    m_epollFd = epoll_create(1);
    m_addToEpoll(nullptr, m_sockFd, EPOLLIN | EPOLLOUT | EPOLLET);

    char buf[BUF_SIZE] = {0};

    m_log() << "Listening...\n";

    uint64_t nextClock = 0;

    while (1) {
        uint64_t curTime = l_curMs(), waitTime = 0;
        if (curTime < nextClock) {
            waitTime = nextClock - curTime;
        } else {
            if (m_tm) {
                m_log() << "tick @ " << curTime << "ms\n";
                m_tm->tick();
            }
            nextClock = curTime + TICK_TIME;
        }

        epoll_event ev;
        int nfds = epoll_wait(m_epollFd, &ev, 1, waitTime); // 1s time
        if (nfds < 0) {
            m_log() << "epoll_wait() error: " << strerror(errno) << '\n';
            return;
        } if (nfds == 0) {
            continue;
        }

        m_log() << "epoll event: ptr = " << ev.data.ptr << ", ev = " << ev.events << "\n";

        if (!ev.data.ptr) {
            // socket
            size_t id = ++m_lastId;
            sockaddr_in addr;
            socklen_t slen = sizeof(addr);
            int clsock = accept(m_sockFd, (sockaddr*) &addr, &slen);
            m_clients[id] = std::make_unique<Client>(Client(this, clsock, id, addr));
            inet_ntop(AF_INET, (const void*) &addr.sin_addr, buf, sizeof(buf));
            m_clients[id]->m_log() << "Client connected from " << buf << ":" << ntohs(addr.sin_port) << "\n";
            m_addToEpoll(m_clients[id].get(), clsock, EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLHUP);
            m_onConnect(m_clients[id].get()); 
            continue;
        }
        if (ev.events & EPOLLIN) {
            Client *cl = (Client*) ev.data.ptr;
            int n = read(cl->m_fd, buf, sizeof(buf));
            if (n > 0)
                m_incoming(cl, std::string_view(buf, n));
        }
        if (ev.events & (EPOLLRDHUP | EPOLLHUP)) {
            Client *cl = (Client*) ev.data.ptr;
            cl->m_log() << "Client disconnect\n";
            for (auto i : m_plugins)
                i->onDisconnect(cl);
            epoll_ctl(m_epollFd, EPOLL_CTL_DEL, cl->m_fd, NULL);
            close(cl->m_fd);
            m_clients.erase(cl->m_id);
        }
    }
}

};
