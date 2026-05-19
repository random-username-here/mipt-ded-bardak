#pragma once

#include "BmServerModule.hpp"
#include "Event.hpp"
#include "RoleMgr.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

using std::string;

namespace modlib {

struct LobbyQueuedInfo {
    size_t clientId = 0;
    string roleId;
    size_t waiting  = 0;
    size_t required = 0;
};

struct LobbyGameStartedInfo {
    uint64_t matchId  = 0;
    size_t   players  = 0;
    size_t   required = 0;
};

struct LobbyGameEndedInfo {
    uint64_t matchId = 0;
    string result; // "winner" or "draw"
    size_t winnerClientId = 0;
    string winnerRoleId;
    size_t players = 0;
    size_t dead    = 0;
};

class Lobby : public BmServerModule {
public:
    using StartCallback = std::function<void()>;

    Event<const LobbyQueuedInfo      &> EvClientQueued;
    Event<const LobbyGameStartedInfo &> EvGameStarted;
    Event<const LobbyGameEndedInfo   &> EvGameEnded;

    virtual bool enqueue(BmClient *client, const ClientRoleInfo &role, StartCallback start) = 0;
    virtual bool isRunning() const = 0;
    virtual bool isQueued(BmClient *client) const = 0;
    virtual bool isActive(BmClient *client) const = 0;
    virtual size_t requiredPlayers() const = 0;

    ~Lobby() override = default;
};

} // namespace modlib
