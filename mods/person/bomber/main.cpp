#include "../person_base.hpp"
#include "RoleMgr.hpp"
#include "Timer.hpp"
#include "modlib_manager.hpp"
#include "bomber_proto.hpp"

#include <cstdlib>
#include <string_view>
#include <unordered_map>
#include <limits>

constexpr uint64_t kBomberType = 2;
constexpr uint64_t kBomberTeam = 2;
constexpr uint64_t kMoveCooldownTicks = 1;
constexpr int kVisionRadius = 4;
static const std::string kBomberRoleID = "bomber";
static const std::string kBomberRolePrefix = "bomber";

constexpr uint64_t kBombType = 3;
constexpr uint64_t kBombTeam = 3;
constexpr uint64_t kBombTimer = 4;

struct Bomb final : public modlib::Building {
    modlib::Map *m_map;
    modlib::Vec2i m_pos;
    size_t m_id;

    bool destroyed;

    Bomb(modlib::Map* map, modlib::Vec2i pos, size_t id)
        : modlib::Building(), m_map(map), m_pos(pos), m_id(id), destroyed(false)
    {}

    modlib::Map *map() override { return m_map; }
    modlib::Tile *tile() override { return m_map->at(m_pos); }
    uint64_t id() override { return m_id; }
    uint64_t type() const override { return kBombType; }
    uint64_t teamId() const override { return kBombTeam; }
    modlib::Vec2i pos() const override { return m_pos; }

    void move(modlib::Vec2i to) override {}
    void destroy() override {
        if (destroyed) {
            return;
        }

        destroyed = true;
        for (auto* unit : m_map->at(m_pos)->units()) {
            unit->destroy();
        }
    }

    void takeDamage(int) override {}
    int  hp() const override { return std::numeric_limits<int>::max(); }
    virtual uint64_t getAssetId() const override { return 0; }
};

struct Bomber final : public PersonBase {
    Bomber(modlib::Map* map, modlib::Vec2i pos, size_t id, modlib::BmClient* client)
        : PersonBase(map, pos, id, client)
    {}

    uint64_t type() const override { return kBomberType; }
    uint64_t teamId() const override { return kBomberTeam; }
    int attackDamage() const override { return 0; }
    virtual uint64_t getAssetId() const override { return 0; };
};

class BomberRole final : public modlib::BmServerModule {
private:
    modlib::RoleMgr* roles_ = nullptr;
    modlib::Map* map_ = nullptr;
    modlib::Timer* timer_ = nullptr;
    std::unordered_map<size_t, Bomber *> bombers_;
    uint64_t tick_ = 0;

    std::unordered_map<Bomb *, size_t> bombs_ticks_;

public:
    std::string_view id() const override { return "dinichthys.bardak.role.bomber"; }
    std::string_view brief() const override { return "Bomber role"; }
    ModVersion version() const override { return ModVersion(0, 0, 1); }

    void onResolveDeps(ModManager* mm) override {
        roles_ = mm->anyOfType<modlib::RoleMgr>();
        map_ = mm->anyOfType<modlib::Map>();
        timer_ = mm->anyOfType<modlib::Timer>();

        if (roles_ == nullptr) {
            throw ModManager::Error("RoleMgr module not found");
        }
        if (map_ == nullptr) {
            throw ModManager::Error("Map module not found");
        }
        if (timer_ == nullptr) {
            throw ModManager::Error("Timer module not found");
        }
    }

    void onDepsResolved(ModManager*) override {
        if (!roles_->registerRole(kBomberRoleID, "Bomber", kBomberRolePrefix, [this](modlib::BmClient* client) {
            select(client);
        })) {
            throw ModManager::Error("failed to register bomber role");
        }

        timer_->setTimer(1, [this] { tick(); }, modlib::Timer::Stage::ON_UPDATE);
    }

    void onSetup(modlib::BmServer* server) override {
        if (!server->registerPrefix(kBomberRolePrefix, this)) {
            throw ModManager::Error("failed to register bomber prefix");
        }
    }

    void onDisconnect(modlib::BmClient* client) override {
        const auto found = bombers_.find(client->id());
        if (found == bombers_.end()) {
            return;
        }

        found->second->destroy();
        bombers_.erase(found);
    }

    void onMessage(modlib::BmClient* client, bmsg::RawMessage msg) override {
        if (!roles_->clientHasRole(client, kBomberRoleID) || !msg.isCorrect()) {
            return;
        }

        const auto found = bombers_.find(client->id());
        if (found == bombers_.end()) {
            return;
        }

        if (msg.header()->type == "move") {
            handleMove(found->second, msg);
        } else if (msg.header()->type == "bomb") {
            handleBomb(found->second);
        }
    }

private:

    void select(modlib::BmClient* client) {
        if (bombers_.count(client->id()) != 0) {
            return;
        }

        auto* bomber = map_->spawn<Bomber>(findSpawn(), client);
        bombers_[client->id()] = bomber;
        sendState(client, bomber);
    }

    void tick() {
        ++tick_;
        for (auto it = bombers_.begin(); it != bombers_.end();) {
            auto* bomber = it->second;
            if (bomber->destroyed()) {
                it = bombers_.erase(it);
                continue;
            }

            sendState(bomber->m_client, bomber);
            sendVision(bomber->m_client, bomber);
            ++it;
        }

        for (auto it = bombs_ticks_.begin(); it != bombs_ticks_.end();) {
            if (it->second <= 1) {
                it->first->destroy();
                it = bombs_ticks_.erase(it);
                continue;
            }

            it->second--;

            ++it;
        }

        timer_->setTimer(1, [this] { tick(); }, modlib::Timer::Stage::ON_UPDATE);
    }

    void handleBomb(Bomber* bomber) {
        auto* bomb = map_->spawn<Bomb>(bomber->pos());
        bombs_ticks_.insert({bomb, kBombTimer});
    }

    void handleMove(Bomber* bomber, bmsg::RawMessage msg) {
        const auto move = bmsg::CL_bomber_move::decode(msg);
        if (!move) {
            return;
        }
        if (std::abs(move->dx) > 1 || std::abs(move->dy) > 1 || (move->dx == 0 && move->dy == 0)) {
            return;
        }
        if (tick_ < bomber->m_nextMoveTick) {
            return;
        }

        const modlib::Vec2i next { bomber->pos().x + move->dx, bomber->pos().y + move->dy };
        auto* tile = map_->at(next);
        if (!bomber->canEnter(tile)) {
            return;
        }

        bomber->move(next);
        bomber->m_nextMoveTick = tick_ + kMoveCooldownTicks;
    }

    modlib::Vec2i findSpawn() const {
        const auto size = map_->size();
        for (int attempts = 0; attempts < 64; ++attempts) {
            modlib::Vec2i pos { std::rand() % size.x, std::rand() % size.y };
            auto* tile = map_->at(pos);
            if (tile != nullptr && !(tile->type() == modlib::Tile::BasicType::Wall)) {
                return pos;
            }
        }

        for (int y = 0; y < size.y; ++y) {
            for (int x = 0; x < size.x; ++x) {
                modlib::Vec2i pos { x, y };
                auto* tile = map_->at(pos);
                if (tile != nullptr && !(tile->type() == modlib::Tile::BasicType::Wall)) {
                    return pos;
                }
            }
        }

        return { 0, 0 };
    }

    void sendState(modlib::BmClient* client, Bomber* bomber) const {
        client->send(bmsg::SV_bomber_tick {});
        client->send(bmsg::SV_bomber_at { bomber->pos().x, bomber->pos().y });
        client->send(bmsg::SV_bomber_hp { bomber->hp() });
    }

    void sendVision(modlib::BmClient* client, Bomber* bomber) const {
        const auto size = map_->size();
        for (int dx = -kVisionRadius; dx <= kVisionRadius; ++dx) {
            for (int dy = -kVisionRadius; dy <= kVisionRadius; ++dy) {
                const modlib::Vec2i pos { bomber->pos().x + dx, bomber->pos().y + dy };
                if (pos.x < 0 || pos.y < 0 || pos.x >= size.x || pos.y >= size.y) {
                    continue;
                }

                auto* tile = map_->at(pos);
                if (tile->type() == modlib::Tile::BasicType::Wall) {
                    client->send(bmsg::SV_bomber_wall { pos.x, pos.y });
                }
                for (auto* unit : tile->units()) {
                    if (unit != bomber) {
                        client->send(bmsg::SV_bomber_sees { pos.x, pos.y, static_cast<uint32_t>(unit->id()) });
                    }
                }
            }
        }
    }
};

extern "C" Mod* modlib_create(ModManager*) {
    return new BomberRole();
}
