#pragma once

#include "AssetManager.hpp"
#include "BmServerModule.hpp"
#include "Roots.hpp"
#include "Timer.hpp"
#include "binmsg.hpp"
#include "combat_grid.hpp"
#include "mage_animator.hpp"
#include "mage_controller.hpp"
#include "mage_proto.hpp"

#include <iostream>
#include <unordered_map>

class MageManager {
    Timer                  *timer_    = nullptr;
    Level                  *map_      = nullptr;
    modlib::RootSystem     *roots_    = nullptr;
    anim::AnimationManager *animator_ = nullptr;
    modlib::AssetManager   *assets_   = nullptr;

    struct MageUtils {
        MageCtl ctl;
        MageAnimator anim;

        MageUtils(
            Level                  *map,
            modlib::RootSystem     *roots,
            BmClient               *client,
            anim::AnimationManager *animator,
            modlib::AssetManager   *assets
        )
            : ctl(map, roots, client)
            , anim(&ctl, animator, assets)
        {}
    };

    std::unordered_map<BmClient *, MageUtils> mages_;
    uint64_t m_tick = 0;

public:
    void setModules(
        Timer                  *timer,
        Level                  *map,
        modlib::RootSystem     *roots,
        anim::AnimationManager *animator,
        modlib::AssetManager   *assets
    )
    {
        timer_    = timer;
        map_      = map;
        roots_    = roots;
        animator_ = animator;
        assets_   = assets;
    }

    void destroy(BmClient *client)
    {
        auto it = mages_.find(client);
        if (it != mages_.end()) {
            it->second.ctl.destroy();
            mages_.erase(it);
            client->send(bmsg::SV_mage_hp{0});
        }
    }

    void resolve()
    {
        timer_->setTimer(1, [this]() { sendState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }

    void receiveMoveCommand(BmClient *client, bmsg::CL_mage_move moveCmd)
    {
        auto it = mages_.find(client);
        if (it == mages_.end()) {
            return;
        }

        it->second.ctl.move(moveCmd.dx, moveCmd.dy, m_tick);
    }

    void receiveUseCommand(BmClient *client, bmsg::CL_mage_use useCmd)
    {
        auto it = mages_.find(client);
        if (it == mages_.end()) {
            return;
        }

        it->second.ctl.useAbility(
            useCmd.ability,
            useCmd.target,
            {useCmd.x, useCmd.y},
            m_tick
        );
    }

    size_t count(BmClient *client) const
    {
        return mages_.count(client);
    }

    void spawnMage(BmClient *client)
    {
        if (mages_.count(client)) {
            std::cerr << "mage with client `" << client->id() << "` was already spawned\n";
            return;
        }

        mages_.try_emplace(client, map_, roots_, client, animator_, assets_);
    }

private:
    void sendState()
    {
        ++m_tick;

        for (auto &[client, mage] : mages_) {
            const Vec2i pos = mage.ctl.pos();

            client->send(bmsg::SV_mage_at{pos.x, pos.y});
            client->send(bmsg::SV_mage_hp{mage.ctl.hp()});

            sendVisible(client, mage.ctl);
            sendInventory(client, mage.ctl.mage()->inventory());

            client->send(bmsg::SV_mage_tick{});
        }

        timer_->setTimer(1, [this]() { sendState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }

    void sendVisible(BmClient *client, MageCtl &ctl)
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
                client->send(bmsg::SV_mage_wall{pos.x, pos.y});
            }

            for (const auto &[id, entity] : tile->getEntityList()) {
                (void)id;

                if (entity == nullptr || entity == ctl.mage()) {
                    continue;
                }

                if (combat_grid::isRoot(entity)) {
                    client->send(bmsg::SV_mage_root{
                        pos.x,
                        pos.y,
                        static_cast<uint32_t>(entity->getID())
                    });
                    continue;
                }

                if (combat_grid::isCombatant(entity)) {
                    client->send(bmsg::SV_mage_enemy{
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
            client->send(bmsg::SV_mage_item{item.id});
        }

        for (const auto &ability : inventory.abilities()) {
            client->send(bmsg::SV_mage_ability{ability.id});
        }
    }
};
