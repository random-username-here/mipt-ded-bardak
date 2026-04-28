#include "../person_base.hpp"
#include "Map.hpp"
#include "ghost_proto.hpp"
#include "RoleMgr.hpp"
#include "Timer.hpp"
#include "modlib_manager.hpp"

#include <cstdlib>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

namespace {

constexpr uint64_t kGhostType = 2;
constexpr uint64_t kGhostTeam = 2;
constexpr uint64_t kMoveCooldownTicks = 1;
constexpr uint64_t kAttackCooldownTicks = 2;
constexpr int kVisionRadius = 4;
constexpr int kAttackDamage = 25;

struct Ghost final : public PersonBase {
    Ghost(modlib::Map* map, modlib::Vec2i pos, size_t id, modlib::BmClient* client)
        : PersonBase(map, pos, id, client)
    {}

    uint64_t type() const override { return kGhostType; }
    uint64_t teamId() const override { return kGhostTeam; }
    int attackDamage() const override { return kAttackDamage; }
    virtual uint64_t getAssetId() const override { return 1; }; 
	virtual void pickUp() override {};
    virtual int weight() const override {return 0;};
    virtual void setWeight(const int weight) override {};
};

class GhostRole final : public modlib::BmServerModule {
public:
    std::string_view id() const override { return "sevsol.bardak.role.ghost"; }
    std::string_view brief() const override { return "Ghost playable role"; }
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
        if (!roles_->registerRole("ghost", "Ghost", "ghost", [this](modlib::BmClient* client) {
            select(client);
        })) {
            throw ModManager::Error("failed to register ghost role");
        }

        timer_->setTimer(1, [this] { tick(); }, modlib::Timer::Stage::ON_UPDATE);
    }

    void onSetup(modlib::BmServer* server) override {
        if (!server->registerPrefix("ghost", this)) {
            throw ModManager::Error("failed to register ghost prefix");
        }
    }

    void onDisconnect(modlib::BmClient* client) override {
        const auto found = ghosts_.find(client->id());
        if (found == ghosts_.end()) {
            return;
        }

        found->second->destroy();
        ghosts_.erase(found);
    }

    void onMessage(modlib::BmClient* client, bmsg::RawMessage msg) override {
        if (!roles_->clientHasRole(client, "ghost") || !msg.isCorrect()) {
            return;
        }

        const auto found = ghosts_.find(client->id());
        if (found == ghosts_.end()) {
            return;
        }

        if (msg.header()->type == "move") {
            handleMove(found->second, msg);
        } else if (msg.header()->type == "attack") {
            handleAttack(found->second, msg);
        }
    }

private:
    modlib::RoleMgr* roles_ = nullptr;
    modlib::Map* map_ = nullptr;
    modlib::Timer* timer_ = nullptr;
    std::unordered_map<size_t, Ghost*> ghosts_;
    uint64_t tick_ = 0;

    void select(modlib::BmClient* client) {
        if (ghosts_.count(client->id()) != 0) {
            return;
        }

        auto* ghost = map_->spawn<Ghost>(findSpawn(), client);
        ghosts_[client->id()] = ghost;
        sendState(client, ghost);
    }

    void tick() {
        ++tick_;
        for (auto it = ghosts_.begin(); it != ghosts_.end();) {
            auto* ghost = it->second;
            if (ghost->destroyed()) {
                it = ghosts_.erase(it);
                continue;
            }

            sendState(ghost->m_client, ghost);
            sendVision(ghost->m_client, ghost);
            ++it;
        }

        timer_->setTimer(1, [this] { tick(); }, modlib::Timer::Stage::ON_UPDATE);
    }

    void handleMove(Ghost* ghost, bmsg::RawMessage msg) {
        const auto move = bmsg::CL_ghost_move::decode(msg);
        if (!move) {
            return;
        }
        if (std::abs(move->dx) > 1 || std::abs(move->dy) > 1 || (move->dx == 0 && move->dy == 0)) {
            return;
        }
        if (tick_ < ghost->m_nextMoveTick) {
            return;
        }

        const modlib::Vec2i next { ghost->pos().x + move->dx, ghost->pos().y + move->dy };
        auto* tile = map_->at(next);
        if (!ghost->canEnter(tile)) {
            return;
        }

        ghost->move(next);
        ghost->m_nextMoveTick = tick_ + kMoveCooldownTicks;
    }

    void handleAttack(Ghost* ghost, bmsg::RawMessage msg) {
        const auto attack = bmsg::CL_ghost_attack::decode(msg);
        if (!attack) {
            return;
        }
        if (tick_ < ghost->m_nextAttackTick) {
            return;
        }

        auto* target = dynamic_cast<modlib::MOB*>(map_->byId(attack->whom));
        if (target == nullptr) {
            return;
        }
        if (std::abs(target->pos().x - ghost->pos().x) > 1 || std::abs(target->pos().y - ghost->pos().y) > 1) {
            return;
        }

        target->takeDamage(ghost->attackDamage());
        ghost->m_nextAttackTick = tick_ + kAttackCooldownTicks;
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

    void sendState(modlib::BmClient* client, Ghost* ghost) const {
        client->send(bmsg::SV_ghost_tick {});
        client->send(bmsg::SV_ghost_at { ghost->pos().x, ghost->pos().y });
        client->send(bmsg::SV_ghost_hp { ghost->hp() });
    }

    void sendVision(modlib::BmClient* client, Ghost* ghost) const {
        const auto size = map_->size();
        for (int dx = -kVisionRadius; dx <= kVisionRadius; ++dx) {
            for (int dy = -kVisionRadius; dy <= kVisionRadius; ++dy) {
                const modlib::Vec2i pos { ghost->pos().x + dx, ghost->pos().y + dy };
                if (pos.x < 0 || pos.y < 0 || pos.x >= size.x || pos.y >= size.y) {
                    continue;
                }

                auto* tile = map_->at(pos);
                if (tile->type() == modlib::Tile::BasicType::Wall) {
                    client->send(bmsg::SV_ghost_wall { pos.x, pos.y });
                }
                for (auto* unit : tile->units()) {
                    if (unit != ghost) {
                        client->send(bmsg::SV_ghost_sees { pos.x, pos.y, static_cast<uint32_t>(unit->id()) });
                    }
                }
            }
        }
    }
};

} // namespace

extern "C" Mod* modlib_create(ModManager*) {
    return new GhostRole();
}
