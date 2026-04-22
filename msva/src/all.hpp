#include "BmServerModule.hpp"
#include "binmsg.hpp"
#include "modlib_manager.hpp"
#include "libpan.h"
#include <cstdint>
#include <netinet/in.h>
#include <unordered_map>

#define ESC_GRY "\x1b[90m"
#define ESC_RED "\x1b[91m"
#define ESC_GRN "\x1b[92m"
#define ESC_YLW "\x1b[93m"
#define ESC_BLU "\x1b[94m"
#define ESC_MGN "\x1b[95m"
#define ESC_CYN "\x1b[96m"
#define ESC_RST "\x1b[0m"

namespace msva {

class Server;

class Client : public modlib::BmClient {
    friend class Server;
    int m_fd;
    std::string m_partialMsg;
    Server *m_server;
    size_t m_id;
    sockaddr_in m_addr;
    size_t m_seq = 1;

    std::ostream &m_log();

    Client(Server *s, int fd, size_t id, sockaddr_in addr) :m_server(s), m_fd(fd), m_id(id), m_addr(addr) {}

public:

    bmsg::Id getMsgId() override { return m_seq++; }
    void send(bmsg::RawMessage msg) override;
    template<typename T>
    void send(const T& msg, uint16_t flags = 0) {
        std::ostringstream oss;
        msg.encode(oss, getMsgId(), flags);
        send(bmsg::RawMessage(oss.str()));
    }
    size_t id() const override { return m_id; }
};

class Server : public modlib::BmServer {
    friend class Client;

    std::unordered_map<size_t, std::unique_ptr<Client>> m_clients;
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

    void m_addToEpoll(Client *cl, int fd, uint32_t flags);
    void m_incoming(Client *cl, std::string_view data);
    void m_processMessage(Client *cl, bmsg::RawMessage msg);
    void m_srvMessage(Client *cl, bmsg::RawMessage msg);
    void m_onConnect(Client *cl);
    std::ostream &m_log();

public:

    bool registerPrefix(std::string_view pref, modlib::BmServerModule *mod) override;
    void listenPrefix(std::string_view pref, modlib::BmServerModule *mod) override;
    void listenAll(modlib::BmServerModule *mod) override;
    void forAllClients(const std::function<void(modlib::BmClient *)> cb) override {
        for (auto &[id, cl] : m_clients)
            cb(cl.get());
    }

    Server(ModManager *mm, PAN *pan);

    void initMods();
    void setPort(size_t port) { m_port = port; }
    void setName(std::string_view name) { m_name = name; }
    void mainloop();

    Server(const Server &s) = delete;
    Server &operator=(const Server &s) = delete;
    //~Server();
};
};
