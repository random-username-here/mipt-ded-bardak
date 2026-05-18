#include "Lobby.hpp"
#include "Map.hpp"
#include "Timer.hpp"
#include "binmsg.hpp"
#include "lobby_proto.hpp"
#include "modlib_manager.hpp"
#include "msva_api.hpp"

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

class LobbyImpl final : public modlib::Lobby {
    enum class State {
        Waiting,
        Running,
    };

    struct QueuedPlayer {
        modlib::BmClient      *client = nullptr;
        modlib::ClientRoleInfo role;
        StartCallback          start;
    };

    struct ActivePlayer {
        modlib::BmClient      *client   = nullptr;
        size_t                 clientId = 0;
        modlib::ClientRoleInfo role;
        modlib::Entity::ID     entity   = 0;
        bool dead = false;
    };

    modlib::Timer *m_timer = nullptr;
    modlib::Level *m_map   = nullptr;
    msva::Server  *m_msva  = nullptr;

    State m_state = State::Waiting;
    std::deque <QueuedPlayer> m_queue;
    std::vector<ActivePlayer> m_active;

    size_t m_requiredPlayers = 2;
    size_t m_startDelayTicks = 5;
    bool m_kickFinishedClients = true;

    modlib::Timer::TimerID m_startTimer = 0;
    bool m_startTimerSet = false;
    uint64_t m_matchId = 0;

public:
    std::string_view id() const override {
        return "ashww.bardak.lobby";
    }

    std::string_view brief() const override {
        return "Match lobby";
    }

    ModVersion version() const override {
        return ModVersion(0, 1, 0);
    }

    void onResolveDeps(ModManager *mm) override {
        m_timer = mm->requireAnyOfType<modlib::Timer>("Lobby needs Timer");
        m_map   = mm->requireAnyOfType<modlib::Level>("Lobby needs Map");
    }

    void onSetup(modlib::BmServer *server) override {
        if (!server->registerPrefix("lobby", this)) {
            throw ModManager::Error("failed to register lobby prefix");
        }

        m_msva = dynamic_cast<msva::Server *>(server);
        readConfig();
    }

    void onDisconnect(modlib::BmClient *client) override {
        removeQueued(client);
        markDead(client, true);
    }

    bool enqueue(modlib::BmClient *client, const modlib::ClientRoleInfo &role, StartCallback start) override {
        if (client == nullptr || !start || isQueued(client) || isActive(client)) {
            return false;
        }

        m_queue.push_back(QueuedPlayer{client, role, std::move(start)});

        const modlib::LobbyQueuedInfo info{
            client->id(),
            role.id,
            m_queue.size(),
            m_requiredPlayers
        };
        EvClientQueued.emit(info);

        client->send(bmsg::SV_lobby_queued{
            static_cast<int32_t>(m_requiredPlayers),
            static_cast<int32_t>(m_queue.size())
        });

        tryStartCountdown();
        return true;
    }

    bool isRunning() const override {
        return m_state == State::Running;
    }

    bool isQueued(modlib::BmClient *client) const override {
        if (client == nullptr) {
            return false;
        }

        return std::any_of(m_queue.begin(), m_queue.end(), [client](const QueuedPlayer &player) {
            return player.client == client;
        });
    }

    bool isActive(modlib::BmClient *client) const override {
        if (client == nullptr) {
            return false;
        }

        return std::any_of(m_active.begin(), m_active.end(), [client](const ActivePlayer &player) {
            return player.clientId == client->id();
        });
    }

    size_t requiredPlayers() const override {
        return m_requiredPlayers;
    }

private:
    void readConfig() {
        if (m_msva == nullptr) return;

        m_requiredPlayers     = std::max<size_t>(2, readSizeConfig("lobby_players",               m_requiredPlayers));
        m_startDelayTicks     =                     readSizeConfig("lobby_start_delay_ticks",     m_startDelayTicks);
        m_kickFinishedClients =                     readBoolConfig("lobby_kick_finished_clients", m_kickFinishedClients);
    }

    size_t readSizeConfig(std::string_view key, size_t fallback) const {
        const auto value = m_msva->configValue(key);
        if (!value || value->empty()) {
            return fallback;
        }

        char *end = nullptr;
        const unsigned long parsed = std::strtoul(value->c_str(), &end, 10);
        if (end == value->c_str() || *end != '\0') {
            return fallback;
        }

        return static_cast<size_t>(parsed);
    }

    bool readBoolConfig(std::string_view key, bool fallback) const {
        const auto value = m_msva->configValue(key);
        if (!value) {
            return fallback;
        }

        if (*value == "true") {
            return true;
        }
        if (*value == "false") {
            return false;
        }

        return fallback;
    }

    void tryStartCountdown() {
        if (m_state != State::Waiting) {
            return;
        }

        if (m_queue.size() < m_requiredPlayers) {
            cancelStartCountdown();
            return;
        }

        if (m_startTimerSet) {
            return;
        }

        if (m_startDelayTicks == 0) {
            startGame();
            return;
        }

        m_startTimer = m_timer->setTimer(
            m_startDelayTicks,
            [this]() {
                m_startTimerSet = false;
                if (m_state == State::Waiting && m_queue.size() >= m_requiredPlayers) {
                    startGame();
                }
            },
            modlib::Timer::Stage::ON_UPDATE
        );
        m_startTimerSet = true;
    }

    void cancelStartCountdown() {
        if (!m_startTimerSet) {
            return;
        }

        m_timer->cancelTimer(m_startTimer);
        m_startTimerSet = false;
    }

    void startGame() {
        cancelStartCountdown();

        if (m_state != State::Waiting || m_queue.size() < m_requiredPlayers) {
            return;
        }

        m_state = State::Running;
        ++m_matchId;
        m_active.clear();

        for (size_t i = 0; i < m_requiredPlayers; ++i) {
            QueuedPlayer player = std::move(m_queue.front());
            m_queue.pop_front();
            activate(std::move(player));
        }

        const modlib::LobbyGameStartedInfo info{
            m_matchId,
            m_active.size(),
            m_requiredPlayers
        };
        EvGameStarted.emit(info);

        broadcast(bmsg::SV_lobby_start{
            static_cast<int32_t>(m_matchId),
            static_cast<int32_t>(m_active.size())
        });

        finishIfEnoughDead();
    }

    void activate(QueuedPlayer player) {
        const auto before = entityIds();
        player.start();

        modlib::Entity    *entity   = findSpawnedEntity(before, player.role);
        modlib::Entity::ID entityId = entity ? entity->getID() : 0;

        m_active.push_back(ActivePlayer{
            player.client,
            player.client->id(),
            player.role,
            entityId,
            false
        });

        if (auto *health = dynamic_cast<EC::Stats::Health *>(entity)) {
            health->EvDeath.subscribe([this, client = player.client]() {
                markDead(client, false);
            });
        }
    }

    std::unordered_set<modlib::Entity::ID> entityIds() const {
        std::unordered_set<modlib::Entity::ID> ids;
        for (const auto &[id, entity] : m_map->getEntityList()) {
            (void)entity;
            ids.insert(id);
        }
        return ids;
    }

    modlib::Entity *findSpawnedEntity(
        const std::unordered_set<modlib::Entity::ID> &before,
        const modlib::ClientRoleInfo                 &role
    ) const {
        modlib::Entity *fallback = nullptr;
        const bmsg::Char64 preferredType(role.prefix);

        for (const auto &[id, entity] : m_map->getEntityList()) {
            if (entity == nullptr || before.count(id) != 0) {
                continue;
            }

            if (entity->getType() == preferredType) {
                return entity;
            }

            if (fallback == nullptr && dynamic_cast<EC::Stats::Health *>(entity) != nullptr) {
                fallback = entity;
            }
        }

        return fallback;
    }

    void markDead(modlib::BmClient *client, bool disconnecting) {
        if (m_state != State::Running || client == nullptr) {
            return;
        }

        auto it = std::find_if(m_active.begin(), m_active.end(), [client](const ActivePlayer &player) {
            return player.clientId == client->id();
        });
        if (it == m_active.end() || it->dead) {
            return;
        }

        it->dead = true;
        if (disconnecting) {
            it->client = nullptr;
        }
        finishIfEnoughDead(disconnecting ? client->id() : 0);
    }

    void finishIfEnoughDead(size_t skipDisconnectClientId = 0) {
        if (m_state != State::Running) {
            return;
        }

        const size_t dead = std::count_if(m_active.begin(), m_active.end(), [](const ActivePlayer &player) {
            return player.dead;
        });

        if (dead < m_requiredPlayers - 1) {
            return;
        }

        finishGame(dead, skipDisconnectClientId);
    }

    void finishGame(size_t dead, size_t skipDisconnectClientId) {
        ActivePlayer *winner = nullptr;
        size_t alive = 0;

        for (ActivePlayer &player : m_active) {
            if (!player.dead) {
                ++alive;
                winner = &player;
            }
        }

        modlib::LobbyGameEndedInfo info;
        info.matchId        = m_matchId;
        info.result         = (alive == 1) ? "winner" : "draw";
        info.winnerClientId = (alive == 1 && winner != nullptr) ? winner->clientId : 0;
        info.winnerRoleId   = (alive == 1 && winner != nullptr) ? winner->role.id : std::string();
        info.players        = m_active.size();
        info.dead           = dead;

        EvGameEnded.emit(info);

        broadcast(bmsg::SV_lobby_finish{
            static_cast<int32_t>(info.matchId),
            info.result,
            static_cast<bmsg::Id>(info.winnerClientId),
            info.winnerRoleId
        });

        auto clientsToDisconnect = activeClientIdsExcept(skipDisconnectClientId);

        m_active.clear();
        m_state = State::Waiting;

        disconnectFinishedClientsLater(std::move(clientsToDisconnect));
    }

    std::vector<size_t> activeClientIdsExcept(size_t skipClientId) const {
        std::vector<size_t> result;
        result.reserve(m_active.size());

        for (const ActivePlayer &player : m_active) {
            if (player.clientId != 0 && player.clientId != skipClientId) {
                result.push_back(player.clientId);
            }
        }

        return result;
    }

    void disconnectFinishedClientsLater(std::vector<size_t> clientIds) {
        if (!m_kickFinishedClients || m_msva == nullptr || clientIds.empty()) {
            tryStartCountdown();
            return;
        }

        m_timer->setTimer(
            1,
            [this, clientIds = std::move(clientIds)]() {
                for (size_t clientId : clientIds) {
                    m_msva->disconnectClient(clientId);
                }

                tryStartCountdown();
            },
            modlib::Timer::Stage::ON_UPDATE_DONE
        );
    }

    void removeQueued(modlib::BmClient *client) {
        if (client == nullptr) return;

        const auto oldSize = m_queue.size();
        m_queue.erase(
            std::remove_if(m_queue.begin(), m_queue.end(), [client](const QueuedPlayer &player) {
                return player.client == client;
            }),
            m_queue.end()
        );

        if (m_queue.size() != oldSize) {
            tryStartCountdown();
        }
    }

    template<typename Message>
    void broadcast(const Message &message) {
        if (server() == nullptr) {
            return;
        }

        server()->forAllClients([&message](modlib::BmClient *client) {
            client->send(message);
        });
    }
};

} // namespace

extern "C" Mod *modlib_create(ModManager *) {
    return new LobbyImpl();
}
