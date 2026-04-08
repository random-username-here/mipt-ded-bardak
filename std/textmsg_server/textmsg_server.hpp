/**
 * \file
 * \brief   Interface between textmsg-based server and plugins
 * \author  Didyk Ivan
 * \date    2026-04-08
 */
#ifndef I_TEXTMSG_SERVER_HPP
#define I_TEXTMSG_SERVER_HPP

#include <cstddef>
#include <string_view>
#include <span>
#include <vector>

namespace textmsg {

class Message;
class Client;
class Server;
class Plugin;

/**
 * \brief Message.
 * This class does not own anything.
 */
struct Message {
    std::string_view prefix;
    std::string_view name;
    std::span<std::string_view> args;
    size_t id = 0;                      ///< Ignored when sending
};

/**
 * \brief Client, from viewpoint of the plugin.
 */
class Client
{
public:

    /** Obtain client's name set with `srv:setUsername()` */
    virtual std::string_view username() const = 0;

    /** Obtain client's unique ID */
    virtual size_t id() const = 0;

    /** Send a message to that client. */
    virtual void send(const Message &msg) = 0;

    /** Kick him, closing connection. He can reconnect again. */
    virtual void kick() = 0;
};

/**
 * \brief Server, from viewpoint of the plugin.
 */
class Server
{
public:
    /** 
     * Obtain a vector of pointers to all client.
     * This will construct a new vector.
     */
    virtual std::vector<Client*> allClients() = 0;

    /**
     * Send message to each client.
     * You should override this to remove temporary vector creation.
     */
    virtual void broadcast(const Message &msg) {
        for (auto i : allClients())
            i->send(msg);
    }

    /**
     * Register prefix to specified plugin.
     * \returns If that prefix was not taken.
     */
    virtual bool registerPrefix(std::string_view prefix, Plugin *self);
};

/**
 * \brief The plugin.
 *
 * This class is independent from plugin loading system. When writting
 * custom plugin, inherit from this and your plugin system's plugin class.
 */
class Plugin
{
public:
    virtual void onServerInit(Server *server) = 0;
    virtual void onMessage(Client *client, const Message &msg) = 0;
};

};

#endif // ifndef I_TEXTMSG_SERVER_HPP
