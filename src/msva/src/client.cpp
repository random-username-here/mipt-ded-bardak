#include "MSVA_client.hpp"
#include "aixlog.hpp"
#include "srv_proto.hpp"
#include <cstdarg>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cmath>
namespace msva {

static void l_panLogger(void *data, const char *fmt, ...) {
    va_list args, copy;
    va_start(args, fmt);
    va_copy(copy, args);
    std::string res;
    res.resize(vsnprintf(nullptr, 0, fmt, copy) + 1);
    vsnprintf(res.data(), res.size(), fmt, args);
    va_end(args);
    LOG(TRACE, "msva/client") << res << "\x1b[0m\n";
}
    
MsvaClient::MsvaClient() {
   pan_init(&m_pan, l_panLogger, true);
}

void MsvaClient::registerHandler(bmsg::Char64 prefix, std::function<void(bmsg::RawMessage)> cb) {
    m_prefMapping[prefix.as_u64] = cb;
}

void MsvaClient::loadProtoDefs(std::filesystem::path p) {
    if (std::filesystem::is_directory(p)) {
        for (auto i : std::filesystem::directory_iterator(p))
            loadProtoDefs(i);
    } else if(std::filesystem::is_regular_file(p)) {
        if (p.extension() == ".pan") {
            LOG(DEBUG, "msva/main") << "Loading protocol definition " << p << "\n";
            pan_loadDefsFromFile(&m_pan, p.c_str());
        }
    }
}

bool MsvaClient::connect(std::string_view name) {
    if(m_state != MsvaClientState::DISCONNECTED)
        return true;

    m_host = "localhost";
    m_port = 3000;
    auto it = name.find_first_of(":");
    if (it != name.npos) {
        m_host = name.substr(0, it);
        m_port = std::atoi(name.substr(it+1).data());
        if (m_port == 0) {
            LOG(ERROR, "msva/client") << "Bad port number " << name.substr(it+1) << '\n';
            return false;
        }
    } else {
        m_host = name;
    }
    if (m_host == "")
        m_host = "localhost";

    LOG(INFO, "msva/client") << "Connecting to " << m_host << ":" << m_port << '\n';
    m_state = MsvaClientState::CONNECTING;

#define ERROR_IF(COND, MSG)\
    do { if(COND) {\
        LOG(ERROR, "msva/client") << MSG << ": " << strerror(errno) << '\n';\
        m_state = MsvaClientState::DISCONNECTED;\
        return false;\
    } } while (0)

    std::ostringstream portStr;
    portStr << m_port;
    addrinfo *res = nullptr;
    int err = getaddrinfo(m_host.data(), portStr.str().data(), nullptr, &res);
    ERROR_IF(err != 0, "Failed to resolve address");

    m_tcp = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_IF(m_tcp < 0, "Failed to create TCP socket");
    m_udp = socket(AF_INET, SOCK_DGRAM, 0);
    ERROR_IF(m_udp < 0, "Failed to create UDP socket");

    int en = 1;
    err = setsockopt(m_tcp, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en));
    ERROR_IF(err < 0, "Failed to set SO_REUSEADDR on tcp socket");
    err = setsockopt(m_udp, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en));
    ERROR_IF(err < 0, "Failed to set SO_REUSEADDR on udp socket");

    bool found = false;
    for (addrinfo *addr = res; addr != nullptr; addr = addr->ai_next) {
        char hostname[INET6_ADDRSTRLEN];
        char servname[32];
        getnameinfo(addr->ai_addr, addr->ai_addrlen, hostname, sizeof(hostname), servname, sizeof(servname), NI_NUMERICHOST | NI_NUMERICSERV);
        LOG(INFO, "msva/client") << "Availiable address: " << hostname << " port " << servname << '\n';

        if (addr->ai_family != AF_INET)
            continue;
        err = ::connect(m_tcp, addr->ai_addr, addr->ai_addrlen);
        if (err < 0) {
            LOG(NOTICE, "msva/client") << "Failed to connect by one address: " << strerror(errno) << '\n';
            addr = addr->ai_next;
            continue;
        }
        memcpy(&m_serverAddr, addr->ai_addr, sizeof(sockaddr_in));
        found = true;
        break;
    }
    ERROR_IF(!found, "Failed to connect by all provided addresses");

    sockaddr tcp_local_addr;
    socklen_t len = sizeof(tcp_local_addr);
    err = getsockname(m_tcp, (sockaddr*) &tcp_local_addr, &len);
    ERROR_IF(err < 0, "Failed to obtain tcp socket's local address");
    
    err = ::bind(m_udp, (const sockaddr*)(&tcp_local_addr), sizeof(tcp_local_addr));
    ERROR_IF(err < 0, "Failed to bind udp socket to same port as tcp one");

    err = ::connect(m_udp, (const sockaddr*) &m_serverAddr, sizeof(m_serverAddr));
    ERROR_IF(err < 0, "Failed to connect udp socket");

    LOG(NOTICE, "msva/client") << "Connected to " << m_host << ":" << m_port << '\n';
    m_state = MsvaClientState::CONNECTED;
    m_buf.resize(4096);

    freeaddrinfo(res);
    return true;
}

void MsvaClient::disconnect() {
    m_state = MsvaClientState::DISCONNECTED;
    close(m_tcp);
    close(m_udp);
    LOG(NOTICE, "msva/client") << "Disconnected\n";
}

bool MsvaClient::poll() {
    if (m_state != MsvaClientState::CONNECTED) return false;

    if (m_id != -1) {
        int64_t now = nsTime();
        if (m_lastTimeSynced + m_syncInterval < now) {
            LOG(TRACE, "msva/client") << "Requst time sync @ " << now << "ns\n";
            send(bmsg::CL_srv_tLocal { now }, bmsg::USE_UDP | bmsg::NO_BUF);
            m_lastTimeSynced = now;
        }
    }

    for (size_t i = 0; i < m_maxMsgsPerFrame; ++i) {
        pollfd pfds[2] = {
            { .fd = m_tcp, .events = POLLIN, .revents = 0 },
            { .fd = m_udp, .events = POLLIN, .revents = 0 }
        };
        int res = ::poll(pfds, 2, 0);
        if (res < 0) {
            LOG(ERROR, "msva/client") << "Poll errored: " << strerror(errno) << '\n';
            disconnect();
            return false;
        }
        if (res == 0) break;
        if ((pfds[0].revents & (POLLHUP | POLLERR)) || (pfds[1].revents & (POLLHUP | POLLERR))) {
            // hang up is strange to happen on udp, but just in case kernel can do that 
            LOG(NOTICE, "msva/client") << "Socket hangup\n";
            disconnect();
            return false;
        }
        if (pfds[0].revents & POLLIN) {
            ssize_t n = read(m_tcp, m_buf.data(), m_buf.size());
            if (n < 0) {
                LOG(ERROR, "msva/client") << "read() failed: " << strerror(errno) << '\n';
                return true;
            }
            m_tcp_inBuffer += std::string_view(m_buf.data(), n);
            size_t r = m_processMessage(m_tcp_inBuffer);
            if (r != 0)
                m_tcp_inBuffer.erase(m_tcp_inBuffer.begin(), m_tcp_inBuffer.begin() + r);
        }
        if (pfds[1].revents & POLLIN) {
            //LOG(TRACE, "msva/client") << "udp event\n";
            struct iovec iov = { .iov_base = m_buf.data(), .iov_len = m_buf.size() };
            struct sockaddr_in addr;
            struct msghdr msg = {
                .msg_name = &addr, .msg_namelen = sizeof(addr),
                .msg_iov = &iov, .msg_iovlen = 1
            };
            ssize_t n = recvmsg(m_udp, &msg, 0);
            //if (memcmp(&addr, &m_serverAddr, sizeof(sockaddr_in)) != 0)
            //    return false; // not ours
            if (n < 0) {
                LOG(ERROR, "msva/client") << "recvmsg() failed: " << strerror(errno) << '\n';
                return true;
            }
            LOG(TRACE, "msva/client") << "recv " << n << " bytes\n";
            if (n < 4) continue; // no id
            // assume our's id
            m_processMessage(std::string_view(m_buf.data() + 4, n - 4));
        }
    }
    return true;
}

size_t MsvaClient::m_processMessage(std::string_view msg) {
    size_t orig = msg.size();
    while (1) {
        bmsg::RawMessage rm(msg);
        if (!rm.isCorrect()) break; // not full body yet
        if (m_loggingEnabled)
            pan_binDump_short(&m_pan, PAN_SERVER, (const void*) rm.data().data(), rm.data().size());
        if (rm.header()->pref == "srv") {
            m_serverMessage(rm);
        } else if (m_prefMapping.count(rm.header()->pref.as_u64)) {
            m_prefMapping.at(rm.header()->pref.as_u64)(rm);
        }
        msg = rm.tail(); 
    }
    return orig - msg.size();
}

void MsvaClient::m_serverMessage(bmsg::RawMessage rm) {
    if (rm.header()->type == "name") {
        auto m = bmsg::SV_srv_name::decode(rm);
        if (m) {
            LOG(INFO, "msva/client") << "Connected to server `" << m->name << "`\n";
            m_srvName = m->name;
        }
    } else if (rm.header()->type == "id") {
        auto m = bmsg::SV_srv_id::decode(rm);
        if (m) {
            LOG(INFO, "msva/client") << "ID on server is " << m->id << '\n';
            m_id = m->id;
        }
    } else if (rm.header()->type == "hasPref") {
        auto m = bmsg::SV_srv_hasPref::decode(rm);
        if (m) {
            LOG(INFO, "msva/client") << "Server has prefix " << m->pref << '\n';
        }
    } else if (rm.header()->type == "tCorrect") {
        auto m = bmsg::SV_srv_tCorrect::decode(rm);
        if (m) {
            int64_t now = nsTime();
            LOG(TRACE, "msva/client") 
                << "Time sync: local=" << m->client_ns 
                << "ns, remote=" << m->server_ns << "ns, now=" << now
                << "ns ==> trip=" << (now - m->client_ns)/2
                << "ns, delta=" << (m->server_ns - m_delay - m->client_ns) << "ns\n";
            m_delay = (m_delay * 1 + std::abs(now - m->client_ns) / 2 * 10) / 11;
            m_timeDelta = (m_timeDelta * 1 + (m->server_ns - m_delay - m->client_ns) * 10) / 11;
        }
    }
}

void MsvaClient::send(bmsg::RawMessage m) {
    pan_binDump_short(&m_pan, PAN_CLIENT, m.data().data(), m.data().size());
    if (m.header()->flags.has(bmsg::USE_UDP)) {
        // TODO: buffer stuff
        uint32_t id = m_id;
        iovec iov[2] = {
            { .iov_base = (void*) &id, .iov_len = 4 },
            { .iov_base = (void*) m.data().data(), .iov_len = m.data().size() }
        };
        msghdr mh = { 
            .msg_name = &m_serverAddr,
            .msg_namelen = sizeof(m_serverAddr),
            .msg_iov = iov,
            .msg_iovlen = 2
        };
        ssize_t err = ::sendmsg(m_udp, &mh, MSG_NOSIGNAL);
        if (err < 0)
            LOG(ERROR, "msva/client") << "UDP sendmsg() failed: " << strerror(errno) << '\n';
    } else {
        ssize_t err = ::send(m_tcp, m.data().data(), m.data().size(), MSG_NOSIGNAL);
        if (err < 0)
            LOG(ERROR, "msva/client") << "TCP send() failed: " << strerror(errno) << '\n';
    }
}

MsvaClient::~MsvaClient() {
    if (m_host.empty()) return; // moved out
    disconnect();
}


};
