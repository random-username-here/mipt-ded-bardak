#pragma once

#include "AssetManager.hpp"
#include "BmServerModule.hpp"
#include "Timer.hpp"
#include "hunter_animator.hpp"
#include "hunter_controller.hpp"
#include "hunter_proto.hpp"
#include "binmsg.hpp"
#include "combat_grid.hpp"

#include <iostream>
#include <unordered_map>

class HunterManager {
    Timer *timer_ = nullptr;
    Level *map_   = nullptr;
    anim::AnimationManager *animator_ = nullptr;
    modlib::AssetManager   *assets_   = nullptr;

    struct HunterUtils {
        HunterCtl ctl;
        HunterAnimator anim;

        HunterUtils(
            Level    *map,
            BmClient *client,
            anim::AnimationManager *animator,
            modlib::AssetManager   *assets
        )
            : ctl(map, client)
            , anim(&ctl, animator, assets)
        {}
    };

    std::unordered_map<BmClient *, HunterUtils> hunters_;
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
        auto it = hunters_.find(client);
        if (it != hunters_.end()) {
            it->second.ctl.destroy();
            hunters_.erase(it);
            client->send(bmsg::SV_hunter_hp{0});
        }
    }

    void resolve()
    {
        timer_->setTimer(1, [this]() { sendState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }

    void receiveMoveCommand(BmClient *client, bmsg::CL_hunter_move moveCmd)
    {
        auto it = hunters_.find(client);
        if (it == hunters_.end()) {
            return;
        }

        it->second.ctl.move(moveCmd.dx, moveCmd.dy, m_tick);
    }

    void receiveUseCommand(BmClient *client, bmsg::CL_hunter_use useCmd)
    {
        auto it = hunters_.find(client);
        if (it == hunters_.end()) {
            return;
        }

        it->second.ctl.useAbility(useCmd.ability, useCmd.target, m_tick);
    }

    size_t count(BmClient *client) const
    {
        return hunters_.count(client);
    }

    void spawnHunter(BmClient *client)
    {
        if (hunters_.count(client)) {
            std::cerr << "hunter with client `" << client->id() << "` was already spawned\n";
            return;
        }

        hunters_.try_emplace(client, map_, client, animator_, assets_);
    }

private:
    void sendState()
    {
        ++m_tick;

        for (auto &[client, hunter] : hunters_) {
            const Vec2i pos = hunter.ctl.pos();

            client->send(bmsg::SV_hunter_at{pos.x, pos.y});
            client->send(bmsg::SV_hunter_hp{hunter.ctl.hp()});

            sendVisible  (client, hunter.ctl);
            sendInventory(client, hunter.ctl.hunter()->inventory());

            client->send(bmsg::SV_hunter_tick{});
        }

        timer_->setTimer(1, [this]() { sendState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }

    void sendVisible(BmClient *client, HunterCtl &ctl)
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
                client->send(bmsg::SV_hunter_wall{pos.x, pos.y});
            }

            for (const auto &[id, entity] : tile->getEntityList()) {
                (void)id;

                if (entity == nullptr || entity == ctl.hunter()) {
                    continue;
                }

                if (combat_grid::isRoot(entity)) {
                    client->send(bmsg::SV_hunter_root{
                        pos.x,
                        pos.y,
                        static_cast<uint32_t>(entity->getID())
                    });
                    continue;
                }

                if (combat_grid::isCombatant(entity)) {
                    client->send(bmsg::SV_hunter_enemy{
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
            client->send(bmsg::SV_hunter_item{item.id});
        }

        for (const auto &ability : inventory.abilities()) {
            client->send(bmsg::SV_hunter_ability{ability.id});
        }
    }
};
