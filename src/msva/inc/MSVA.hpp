/**
 * \file
 * \brief Modular server
 * \author Ivan Didyk
 * \date most part done at ~2026-04-20, compressed at 2026-10-02
 */
#pragma once
#include "libpan.h"
#include "modlib_mod.hpp"
#include "binmsg.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <netinet/in.h>
#include <optional>
#include <sstream>
#include <string_view>

namespace msva {

class MsvaModule;
class MsvaUser;
class MsvaServer;

int64_t nsTime();

/**
 * \brief Client, from module's point of view
 */
class MsvaUser {
    friend class MsvaServer;

    MsvaServer *m_server;
    size_t m_id;
    sockaddr_in m_addr;
    std::string m_tag;

    // tcp
    std::string m_tcp_inBuffer;
    int m_tcp;

    // udp
    int m_udp; // the same udp is shared

    size_t m_seq = 1;
    bool m_disconnecting = false;

    MsvaUser(MsvaServer *s, int tcp, int udp, size_t id, sockaddr_in addr);


public:

    /** Send already encoded message to him */
    void send(bmsg::RawMessage msg);

    /* Get ID for new message */

    bmsg::Id getMsgId() { return m_seq++; }

    /** Encode given message using `.encode(std::ostream, id, flags)` function, and send it. */
    template<typename T>
    void send(const T& msg, uint16_t flags = 0) {
        std::ostringstream oss;
        msg.encode(oss, getMsgId(), flags);
        send(bmsg::RawMessage(oss.str()));
    }

    /** Client's ID */
    size_t id() const { return m_id; };

    virtual sockaddr_in addr() const { return m_addr; };
};

/**
 * \brief The server.
 */
class MsvaServer {

    friend class MsvaUser;

    std::unordered_map<size_t, std::unique_ptr<MsvaUser>> m_clients;
    std::unordered_map<uint64_t, MsvaModule*> m_prefMapping;
    std::unordered_map<uint64_t, std::vector<MsvaModule*>> m_listeners;
    std::vector<MsvaModule*> m_listenersForAll;
    std::vector<MsvaModule*> m_plugins;

    size_t m_lastId = 0;
    int m_tcpSockFd, m_epollFd, m_udpSockFd;

    PAN *m_pan;
    ModManager *m_mm;

    std::string m_name;
    size_t m_port;
    size_t m_tickTime;
    std::unordered_map<std::string, std::string> m_config;

    void m_addToEpoll(void *cl, int fd, uint32_t flags);
    void m_incoming(MsvaUser *cl, std::string_view data);
    void m_processMessage(MsvaUser *cl, bmsg::RawMessage msg);
    void m_srvMessage(MsvaUser *cl, bmsg::RawMessage msg);
    void m_onConnect(MsvaUser *cl);
    void m_disconnect(MsvaUser *cl);

public:
    
    /** 
     * \brief Become responsible for given message prefix.
     *
     * This means what this module will implement the thing written in spec
     * for that prefix. Only one mod can be responsible for one prefix.
     * One mod can be responsible for multiple prefixes.
     *
     * List of prefixes for which this function was called will be sent to client.
     */
    bool registerPrefix(std::string_view pref, MsvaModule *mod);

    /** 
     * \brief Listen given prefix, as an "observer". 
     *
     * This is mostly for loggers, statistics, etc. You should not "modify"
     * game state here.
     */
    void listenPrefix(std::string_view pref, MsvaModule *mod);

    /**
     * \brief Listen to all prefixes.
     *
     * This is for loggers, statistics collection, debugging, etc.
     */
    void listenAll(MsvaModule *mod);

    /** Do something for every client connected. */
    void forAllClients(const std::function<void(MsvaUser *)> cb) {
        for (auto &[id, cl] : m_clients) {
            if (!cl->m_disconnecting) {
                cb(cl.get());
            }
        }
    }

    MsvaServer(ModManager *mm, PAN *pan);

    void initMods();
    void setPort(size_t port) { m_port = port; }
    void setName(std::string_view name) { m_name = name; }
    void setTickTime(size_t time) { m_tickTime = time; }
    void setConfigValue(std::string_view key, std::string_view value);
    std::optional<std::string> configValue(std::string_view key) const;
    void disconnectClient(MsvaUser *client);
    void disconnectClient(size_t clientId);
    void mainloop();

    MsvaServer(const MsvaServer &s) = delete;
    MsvaServer &operator=(const MsvaServer &s) = delete;

};

/**
 * \brief Plugin which knows about networking.
 * To obtain `ModManager` here use `onResolveDeps/...` callbacks from `Mod`.
 */
class MsvaModule : public Mod {
    MsvaServer *m_server;
public:

    MsvaServer *server() const { return m_server; }
    void setServer(MsvaServer *server) { m_server = server; }

    /** Server is starting, register prefixes. */
    virtual void onSetup(MsvaServer *) {};

    /** New client connected. */
    virtual void onConnect(MsvaUser *) {};

    /** Someone disconnected. */
    virtual void onDisconnect(MsvaUser *) {};

    /** That client sent that message. */
    virtual void onMessage(MsvaUser *, bmsg::RawMessage) {};

    /** One iteration of the mainloop */
    virtual void onIteration() {}

    virtual ~MsvaModule() {};
};


};
