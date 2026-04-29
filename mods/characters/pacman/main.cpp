#include "../person_base.hpp"
#include "RoleMgr.hpp"
#include "Timer.hpp"
#include "modlib_manager.hpp"
#include "pacman_proto.hpp"

#include <cstdlib>
#include <string_view>
#include <unordered_map>

constexpr uint64_t kPacmanType = 1;
constexpr uint64_t kPacmanTeam = 1;
constexpr uint64_t kMoveCooldownTicks = 1;
constexpr int kVisionRadius = 4;

struct Pacman final : public PersonBase {
    Pacman(modlib::Map* map, modlib::Vec2i pos, size_t id, modlib::BmClient* client)
        : PersonBase(map, pos, id, client)
    {}

    uint64_t type() const override { return kPacmanType; }
    uint64_t teamId() const override { return kPacmanTeam; }
    int attackDamage() const override { return 0; }
    virtual uint64_t getAssetId() const override { return 0; }; 
};

class PacmanRole final : public modlib::BmServerModule {
public:
    std::string_view id() const override { return "sevsol.bardak.role.pacman"; }
    std::string_view brief() const override { return "Pac-Man role"; }
    ModVersion version() const override { return ModVersion(0, 1, 0); }

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
        if (!roles_->registerRole("pacman", "Pac-Man", "pacman", [this](modlib::BmClient* client) {
            select(client);
        })) {
            throw ModManager::Error("failed to register pacman role");
        }

        timer_->setTimer(1, [this] { tick(); }, modlib::Timer::Stage::ON_UPDATE);
    }

    void onSetup(modlib::BmServer* server) override {
        if (!server->registerPrefix("pacman", this)) {
            throw ModManager::Error("failed to register pacman prefix");
        }
    }

    void onDisconnect(modlib::BmClient* client) override {
        const auto found = pacmen_.find(client->id());
        if (found == pacmen_.end()) {
            return;
        }

        found->second->destroy();
        pacmen_.erase(found);
    }

    void onMessage(modlib::BmClient* client, bmsg::RawMessage msg) override {
        if (!roles_->clientHasRole(client, "pacman") || !msg.isCorrect()) {
            return;
        }

        const auto found = pacmen_.find(client->id());
        if (found == pacmen_.end()) {
            return;
        }

        if (msg.header()->type == "move") {
            handleMove(found->second, msg);
        }
    }

private:
    modlib::RoleMgr* roles_ = nullptr;
    modlib::Map* map_ = nullptr;
    modlib::Timer* timer_ = nullptr;
    std::unordered_map<size_t, Pacman*> pacmen_;
    uint64_t tick_ = 0;

    void select(modlib::BmClient* client) {
        if (pacmen_.count(client->id()) != 0) {
            return;
        }

        auto* pacman = map_->spawn<Pacman>(findSpawn(), client);
        pacmen_[client->id()] = pacman;
        sendState(client, pacman);
    }

    void tick() {
        ++tick_;
        for (auto it = pacmen_.begin(); it != pacmen_.end();) {
            auto* pacman = it->second;
            if (pacman->destroyed()) {
                it = pacmen_.erase(it);
                continue;
            }

            sendState(pacman->m_client, pacman);
            sendVision(pacman->m_client, pacman);
            ++it;
        }

        timer_->setTimer(1, [this] { tick(); }, modlib::Timer::Stage::ON_UPDATE);
    }

    void handleMove(Pacman* pacman, bmsg::RawMessage msg) {
        const auto move = bmsg::CL_pacman_move::decode(msg);
        if (!move) {
            return;
        }
        if (std::abs(move->dx) > 1 || std::abs(move->dy) > 1 || (move->dx == 0 && move->dy == 0)) {
            return;
        }
        if (tick_ < pacman->m_nextMoveTick) {
            return;
        }

        const modlib::Vec2i next { pacman->pos().x + move->dx, pacman->pos().y + move->dy };
        auto* tile = map_->at(next);
        if (!pacman->canEnter(tile)) {
            return;
        }

        pacman->move(next);
        pacman->m_nextMoveTick = tick_ + kMoveCooldownTicks;
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

    void sendState(modlib::BmClient* client, Pacman* pacman) const {
        client->send(bmsg::SV_pacman_tick {});
        client->send(bmsg::SV_pacman_at { pacman->pos().x, pacman->pos().y });
        client->send(bmsg::SV_pacman_hp { pacman->hp() });
    }

    void sendVision(modlib::BmClient* client, Pacman* pacman) const {
        const auto size = map_->size();
        for (int dx = -kVisionRadius; dx <= kVisionRadius; ++dx) {
            for (int dy = -kVisionRadius; dy <= kVisionRadius; ++dy) {
                const modlib::Vec2i pos { pacman->pos().x + dx, pacman->pos().y + dy };
                if (pos.x < 0 || pos.y < 0 || pos.x >= size.x || pos.y >= size.y) {
                    continue;
                }

                auto* tile = map_->at(pos);
                if (tile->type() == modlib::Tile::BasicType::Wall) {
                    client->send(bmsg::SV_pacman_wall { pos.x, pos.y });
                }
                for (auto* unit : tile->units()) {
                    if (unit != pacman) {
                        client->send(bmsg::SV_pacman_sees { pos.x, pos.y, static_cast<uint32_t>(unit->id()) });
                    }
                }
            }
        }
    }
};

extern "C" Mod* modlib_create(ModManager*) {
    return new PacmanRole();
}
