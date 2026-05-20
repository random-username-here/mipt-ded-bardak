#pragma once

#include "BmServerModule.hpp"
#include "Timer.hpp"
#include "binmsg.hpp"
#include "priest_animator.hpp"
#include "priest_controller.hpp"
#include "../proto/generated/priest_proto.hpp"
#include "combat_grid.hpp"

#include <iostream>
#include <unordered_map>

class PriestManager {
    Timer *timer_ = nullptr;
    Level *map_   = nullptr;
    anim::AnimationManager *animator_ = nullptr;
    modlib::AssetManager   *assets_   = nullptr;

    struct PriestUtils {
        PriestCtrl ctl;
        PriestAnimator anim;

        PriestUtils(
            Level    *map,
            BmClient *client,
            Timer* timer,
            anim::AnimationManager *animator,
            modlib::AssetManager   *assets
        )
            : ctl(map, client, timer)
            , anim(&ctl, animator, assets)
        {}
    };

    std::unordered_map<BmClient *, PriestUtils> priests_;
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
        auto it = priests_.find(client);
        if (it != priests_.end()) {
            it->second.ctl.destroy();
            priests_.erase(it);
            client->send(bmsg::SV_priest_hp{0});
        }
    }

    void resolve()
    {
        timer_->setTimer(1, [this]() { sendState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }

    void receiveMoveCommand(BmClient *client, bmsg::CL_priest_move moveCmd)
    {
        auto it = priests_.find(client);
        if (it == priests_.end()) {
            return;
        }

        it->second.ctl.move(moveCmd.dx, moveCmd.dy);
    }

    void receiveUseCommand(BmClient *client, bmsg::CL_priest_use useCmd)
    {
        auto it = priests_.find(client);
        if (it == priests_.end()) {
            return;
        }

        it->second.ctl.useAbility(useCmd.ability, useCmd.target);
    }

    size_t count(BmClient *client) const
    {
        return priests_.count(client);
    }

    void spawnPriest(BmClient *client)
    {
        if (priests_.count(client)) {
            std::cerr << "priest with client `" << client->id() << "` was already spawned\n";
            return;
        }

        priests_.try_emplace(client, map_, client, timer_, animator_, assets_);
    }

private:
    void sendState()
    {
        ++m_tick;
        const auto size = map_->getSize();

        for (auto &[client, priest] : priests_) {
            const Vec2i pos = priest.ctl.pos();

            client->send(bmsg::SV_priest_at{pos.x, pos.y});
            client->send(bmsg::SV_priest_hp{priest.ctl.hp()});

            sendVisible(client, priest.ctl);
            sendInventory(client, priest.ctl.priest()->inventory());

            client->send(bmsg::SV_priest_tick{});
        }

        timer_->setTimer(1, [this]() { sendState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }

    void sendInventory(BmClient *client, const modlib::Inventory &inventory)
    {
        for (const auto &item : inventory.items()) {
            client->send(bmsg::SV_priest_item{item.id});
        }

        for (const auto &ability : inventory.abilities()) {
            client->send(bmsg::SV_priest_ability{ability.id});
        }
    }

    void sendVisible(BmClient *client, PriestCtrl &ctl)
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
                client->send(bmsg::SV_priest_wall{pos.x, pos.y});
            }

            for (const auto &[id, entity] : tile->getEntityList()) {
                (void)id;

                if (entity == nullptr || entity == ctl.priest()) {
                    continue;
                }

                if (combat_grid::isRoot(entity)) {
                    client->send(bmsg::SV_priest_root{
                        pos.x,
                        pos.y,
                        static_cast<uint32_t>(entity->getID())
                    });
                    continue;
                }

                if (combat_grid::isCombatant(entity)) {
                    client->send(bmsg::SV_priest_enemy{
                        pos.x,
                        pos.y,
                        static_cast<uint32_t>(entity->getID()),
                        combat_grid::entityTypeName(entity)
                    });
                }
            }
        }
    }
};
