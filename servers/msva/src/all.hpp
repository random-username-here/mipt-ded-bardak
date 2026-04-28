#include "BmServerModule.hpp"
#include "binmsg.hpp"
#include "modlib_manager.hpp"
#include "libpan.h"
#include <cstdint>
#include <fstream>
#include <netinet/in.h>
#include <optional>
#include <sstream>
#include <unordered_map>
#include "Timer.hpp"
#include "msva_api.hpp"

#define ESC_GRY "\x1b[90m"
#define ESC_RED "\x1b[91m"
#define ESC_GRN "\x1b[92m"
#define ESC_YLW "\x1b[93m"
#define ESC_BLU "\x1b[94m"
#define ESC_MGN "\x1b[95m"
#define ESC_CYN "\x1b[96m"
#define ESC_RST "\x1b[0m"

namespace msva {

class ServerImpl;

class ClientImpl : public Client {
    friend class ServerImpl;
    int m_fd;
    std::string m_partialMsg;
    ServerImpl *m_server;
    size_t m_id;
    sockaddr_in m_addr;
    size_t m_seq = 1;


    ClientImpl(ServerImpl *s, int fd, size_t id, sockaddr_in addr) :m_server(s), m_fd(fd), m_id(id), m_addr(addr) {}

public:

    LogStream logg();

    bmsg::Id getMsgId() override { return m_seq++; }
    void send(bmsg::RawMessage msg) override;
    template<typename T>
    void send(const T& msg, uint16_t flags = 0) {
        std::ostringstream oss;
        msg.encode(oss, getMsgId(), flags);
        send(bmsg::RawMessage(oss.str()));
    }
    size_t id() const override { return m_id; }

    sockaddr_in addr() const override { return m_addr; };
};

class ServerImpl : public Server {
    friend class ClientImpl;

    std::unordered_map<size_t, std::unique_ptr<ClientImpl>> m_clients;
    std::unordered_map<uint64_t, modlib::BmServerModule*> m_prefMapping;
    std::unordered_map<uint64_t, std::vector<modlib::BmServerModule*>> m_listeners;
    std::vector<modlib::BmServerModule*> m_listenersForAll;
    std::vector<modlib::BmServerModule*> m_plugins;
    size_t m_lastId = 0;
    int m_sockFd, m_epollFd;
    PAN *m_pan;
    ModManager *m_mm;
    std::string m_name;
    size_t m_port;
    modlib::Timer *m_tm;
    size_t m_tickTime;
    LogFunc m_logCallback;
    bool m_ttylog = true;
    std::optional<std::ofstream> m_fileLogger;

    void m_addToEpoll(ClientImpl *cl, int fd, uint32_t flags);
    void m_incoming(ClientImpl *cl, std::string_view data);
    void m_processMessage(ClientImpl *cl, bmsg::RawMessage msg);
    void m_srvMessage(ClientImpl *cl, bmsg::RawMessage msg);
    void m_onConnect(ClientImpl *cl);

public:
    
    LogStream logg();

    bool registerPrefix(std::string_view pref, modlib::BmServerModule *mod) override;
    void listenPrefix(std::string_view pref, modlib::BmServerModule *mod) override;
    void listenAll(modlib::BmServerModule *mod) override;
    void forAllClients(const std::function<void(modlib::BmClient *)> cb) override {
        for (auto &[id, cl] : m_clients)
            cb(cl.get());
    }

    ServerImpl(ModManager *mm, PAN *pan);

    void initMods();
    void setPort(size_t port) { m_port = port; }
    void setName(std::string_view name) { m_name = name; }
    void setTickTime(size_t time) { m_tickTime = time; }
    void setLogCallback(LogFunc fn) override { m_logCallback = fn; }
    LogFunc logCallback() override { return m_logCallback; };
    void setTTYLogs(bool enabled) override { m_ttylog = enabled; };
    void setLogFile(const std::string &s) { m_fileLogger = std::ofstream(s); }
    void mainloop();

    ServerImpl(const ServerImpl &s) = delete;
    ServerImpl &operator=(const ServerImpl &s) = delete;
};
};
