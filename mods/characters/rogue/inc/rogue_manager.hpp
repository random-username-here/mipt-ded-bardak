#pragma once

#include "AssetManager.hpp"
#include "BmServerModule.hpp"
#include "Timer.hpp"
#include "binmsg.hpp"
#include "combat_grid.hpp"
#include "rogue_animator.hpp"
#include "rogue_controller.hpp"
#include "rogue_proto.hpp"

#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>

class RogueManager {
    Timer *timer_ = nullptr;
    Level *map_   = nullptr;
    anim::AnimationManager *animator_ = nullptr;
    modlib::AssetManager   *assets_   = nullptr;

    struct PendingUse {
        BmClient *client = nullptr;
        std::string ability;
        size_t target = 0;
    };

    struct RogueUtils {
        RogueCtl ctl;
        RogueAnimator anim;

        RogueUtils(
            Level    *map,
            BmClient *client,
            anim::AnimationManager *animator,
            modlib::AssetManager   *assets
        )
            : ctl(map, client)
            , anim(&ctl, animator, assets)
        {}
    };

    std::unordered_map<BmClient *, RogueUtils> rogues_;
    std::vector<PendingUse> m_pendingUses;
    uint64_t m_tick = 0;

public:
    void setModules(
        Timer *timer,
        Level *map,
        anim::AnimationManager *animator,
        modlib::AssetManager   *assets
    )
    {
        timer_    = timer;
        map_      = map;
        animator_ = animator;
        assets_   = assets;
    }

    void destroy(BmClient *client)
    {
        auto it = rogues_.find(client);
        if (it != rogues_.end()) {
            it->second.ctl.destroy();
            rogues_.erase(it);
            client->send(bmsg::SV_rogue_hp{0});
        }
    }

    void resolve()
    {
        timer_->setTimer(
            1,
            [this]() {
                processPendingUses();
            },
            modlib::Timer::Stage::ON_UPDATE,
            modlib::Timer::Type::CYCLE
        );

        timer_->setTimer(1, [this]() { sendState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }

    void receiveMoveCommand(BmClient *client, bmsg::CL_rogue_move moveCmd)
    {
        auto it = rogues_.find(client);
        if (it == rogues_.end()) {
            return;
        }

        it->second.ctl.move(moveCmd.dx, moveCmd.dy, m_tick);
    }

    void receiveUseCommand(BmClient *client, bmsg::CL_rogue_use useCmd)
    {
        if (rogues_.find(client) == rogues_.end()) {
            return;
        }

        m_pendingUses.push_back(PendingUse{
            .client  = client,
            .ability = std::string(useCmd.ability),
            .target  = useCmd.target,
        });
    }

    size_t count(BmClient *client) const
    {
        return rogues_.count(client);
    }

    void spawnRogue(BmClient *client)
    {
        if (rogues_.count(client)) {
            std::cerr << "rogue with client `" << client->id() << "` was already spawned\n";
            return;
        }

        rogues_.try_emplace(client, map_, client, animator_, assets_);
    }

private:
    void processPendingUses()
    {
        std::vector<PendingUse> pending;
        pending.swap(m_pendingUses);

        for (const PendingUse &cmd : pending) {
            auto it = rogues_.find(cmd.client);
            if (it == rogues_.end()) {
                continue;
            }

            it->second.ctl.useAbility(cmd.ability, cmd.target, m_tick);
        }
    }

    void sendState()
    {
        ++m_tick;

        for (auto &[client, rogue] : rogues_) {
            const Vec2i pos = rogue.ctl.pos();

            client->send(bmsg::SV_rogue_at{pos.x, pos.y});
            client->send(bmsg::SV_rogue_hp{rogue.ctl.hp()});

            sendVisible(client, rogue.ctl);
            sendInventory(client, rogue.ctl.rogue()->inventory());

            client->send(bmsg::SV_rogue_tick{});
        }

        timer_->setTimer(1, [this]() { sendState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }

    void sendVisible(BmClient *client, RogueCtl &ctl)
    {
        const Vec2i selfPos = ctl.pos();
        const Vec2i size = map_->getSize();

        for (const Vec2i off : combat_grid::visibleOffsets()) {
            const Vec2i pos{selfPos.x + off.x, selfPos.y + off.y};

            if (pos.x < 0 || pos.y < 0 || pos.x >= size.x || pos.y >= size.y) {
                continue;
            }

            Tile *tile = map_->getTile(pos);
            if (tile == nullptr) {
                continue;
            }

            if (tile->getType() == Tile::BasicTypes::WALL) {
                client->send(bmsg::SV_rogue_wall{pos.x, pos.y});
            }

            for (const auto &[id, entity] : tile->getEntityList()) {
                (void)id;

                if (entity == nullptr || entity == ctl.rogue()) {
                    continue;
                }

                if (combat_grid::isRoot(entity)) {
                    client->send(bmsg::SV_rogue_root{
                        pos.x,
                        pos.y,
                        static_cast<uint32_t>(entity->getID())
                    });
                    continue;
                }

                if (combat_grid::isCombatant(entity)) {
                    client->send(bmsg::SV_rogue_enemy{
                        pos.x,
                        pos.y,
                        static_cast<uint32_t>(entity->getID()),
                        combat_grid::entityTypeName(entity)
                    });
                }
            }
        }
    }

    void sendInventory(BmClient *client, const modlib::Inventory &inventory)
    {
        for (const auto &item : inventory.items()) {
            client->send(bmsg::SV_rogue_item{item.id});
        }

        for (const auto &ability : inventory.abilities()) {
            client->send(bmsg::SV_rogue_ability{ability.id});
        }
    }
};
