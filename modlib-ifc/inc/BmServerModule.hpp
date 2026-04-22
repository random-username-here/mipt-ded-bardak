/**
 * \file
 * \brief Server module API standard
 * \author Ivan Didyk
 * \date 2026-04-20
 */
#pragma once
#include "modlib_mod.hpp"
#include "binmsg.hpp"
#include <cstdint>
#include <functional>
#include <sstream>

namespace modlib {

class BmServerModule;
class BmClient;
class BmRawMessage;

/**
 * \brief Client, from module's point of view
 */
class BmClient {
public:

    /** Send already encoded message to him */
    virtual void send(bmsg::RawMessage msg) = 0;

    /* Get ID for new message */
    virtual bmsg::Id getMsgId() { return 0; }

    /** Encode given message using `.encode(std::ostream, id, flags)` function, and send it. */
    template<typename T>
    void send(const T& msg, uint16_t flags = 0) {
        std::ostringstream oss;
        msg.encode(oss, getMsgId(), flags);
        send(bmsg::RawMessage(oss.str()));
    }

    /** Client's ID */
    virtual size_t id() const = 0;

    //virtual void kick() = 0;
    //virtual std::string_view username() = 0;
};

/**
 * \brief Server, from module's POV
 *
 * Note what this does not provide access to mod manager -- 
 * mod manager is given to mod by modlib in `on...` callbacks.
 */
class BmServer {
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
    virtual bool registerPrefix(std::string_view pref, BmServerModule *mod) = 0;

    /** 
     * \brief Listen given prefix, as an "observer". 
     *
     * This is mostly for loggers, statistics, etc. You should not "modify"
     * game state here.
     */
    virtual void listenPrefix(std::string_view pref, BmServerModule *mod) = 0;

    /**
     * \brief Listen to all prefixes.
     *
     * This is for loggers, statistics collection, debugging, etc.
     */
    virtual void listenAll(BmServerModule *mod) = 0;

    /** Do something for every client connected. */
    virtual void forAllClients(const std::function<void(BmClient* client)> cb) = 0;
};

/**
 * \brief Plugin which knows about networking.
 * To obtain `ModManager` here use `onResolveDeps/...` callbacks from `Mod`.
 */
class BmServerModule : public Mod {
    BmServer *m_server;
public:

    BmServer *server() const { return m_server; }
    void setServer(BmServer *server) { m_server = server; }

    /** Server is starting, register prefixes. */
    virtual void onSetup(BmServer *server) {};

    /** New client connected. */
    virtual void onConnect(BmClient *client) {};

    /** Someone disconnected. */
    virtual void onDisconnect(BmClient *client) {};

    /** That client sent that message. */
    virtual void onMessage(BmClient *client, bmsg::RawMessage msg) {};

    virtual ~BmServerModule() {};
};

};
