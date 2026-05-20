#pragma once

#include "BmServerModule.hpp"
#include "Timer.hpp"
#include "binmsg.hpp"
#include "paladin_animator.hpp"
#include "paladin_controller.hpp"
#include "paladin_proto.hpp"
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

    std::unordered_map<BmClient *, PriestUtils> paladins_;
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
        auto it = paladins_.find(client);
        if (it != paladins_.end()) {
            it->second.ctl.destroy();
            paladins_.erase(it);
            client->send(bmsg::SV_paladin_hp{0});
        }
    }

    void resolve()
    {
        timer_->setTimer(1, [this]() { sendState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }

    void receiveMoveCommand(BmClient *client, bmsg::CL_paladin_move moveCmd)
    {
        auto it = paladins_.find(client);
        if (it == paladins_.end()) {
            return;
        }

        it->second.ctl.move(moveCmd.dx, moveCmd.dy);
    }

    void receiveUseCommand(BmClient *client, bmsg::CL_paladin_use useCmd)
    {
        auto it = paladins_.find(client);
        if (it == paladins_.end()) {
            return;
        }

        it->second.ctl.useAbility(useCmd.ability, useCmd.target);
    }

    size_t count(BmClient *client) const
    {
        return paladins_.count(client);
    }

    void spawnPriest(BmClient *client)
    {
        if (paladins_.count(client)) {
            std::cerr << "paladin with client `" << client->id() << "` was already spawned\n";
            return;
        }

        paladins_.try_emplace(client, map_, client, timer_, animator_, assets_);
    }

private:
    void sendState()
    {
        ++m_tick;
        const auto size = map_->getSize();

        for (auto &[client, paladin] : paladins_) {
            const Vec2i pos = paladin.ctl.pos();

            client->send(bmsg::SV_paladin_at{pos.x, pos.y});
            client->send(bmsg::SV_paladin_hp{paladin.ctl.hp()});

            sendVisible(client, paladin.ctl);
            sendInventory(client, paladin.ctl.paladin()->inventory());

            client->send(bmsg::SV_paladin_tick{});
        }

        timer_->setTimer(1, [this]() { sendState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }

    void sendInventory(BmClient *client, const modlib::Inventory &inventory)
    {
        for (const auto &item : inventory.items()) {
            client->send(bmsg::SV_paladin_item{item.id});
        }

        for (const auto &ability : inventory.abilities()) {
            client->send(bmsg::SV_paladin_ability{ability.id});
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
                client->send(bmsg::SV_paladin_wall{pos.x, pos.y});
            }

            for (const auto &[id, entity] : tile->getEntityList()) {
                (void)id;

                if (entity == nullptr || entity == ctl.paladin()) {
                    continue;
                }

                if (combat_grid::isRoot(entity)) {
                    client->send(bmsg::SV_paladin_root{
                        pos.x,
                        pos.y,
                        static_cast<uint32_t>(entity->getID())
                    });
                    continue;
                }

                if (combat_grid::isCombatant(entity)) {
                    client->send(bmsg::SV_paladin_enemy{
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
