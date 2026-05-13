#pragma once

#include "BmServerModule.hpp"
#include "Timer.hpp"
#include "binmsg.hpp"
#include "tank_animator.hpp"
#include "tank_controller.hpp"
#include "tank_proto.hpp"
#include "combat_grid.hpp"

#include <iostream>
#include <unordered_map>

class TankManager {
    Timer *timer_ = nullptr;
    Level *map_   = nullptr;
    anim::AnimationManager *animator_ = nullptr;
    modlib::AssetManager   *assets_   = nullptr;

    struct TankUtils {
        TankCtl ctl;
        TankAnimator anim;

        TankUtils(
            Level    *map,
            BmClient *client,
            anim::AnimationManager *animator,
            modlib::AssetManager   *assets
        )
            : ctl(map, client)
            , anim(&ctl, animator, assets)
        {}
    };

    std::unordered_map<BmClient *, TankUtils> tanks_;
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
        auto it = tanks_.find(client);
        if (it != tanks_.end()) {
            it->second.ctl.destroy();
            tanks_.erase(it);
            client->send(bmsg::SV_tank_hp{0});
        }
    }

    void resolve()
    {
        timer_->setTimer(1, [this]() { sendState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }

    void receiveMoveCommand(BmClient *client, bmsg::CL_tank_move moveCmd)
    {
        auto it = tanks_.find(client);
        if (it == tanks_.end()) return;
        it->second.ctl.move(m_tick);
    }

    void receiveRotateCommand(BmClient *client, bmsg::CL_tank_rotate rotateCmd)
    {
        auto it = tanks_.find(client);
        if (it == tanks_.end()) return;
        it->second.ctl.rotate(rotateCmd.dir, m_tick);
    }

    void receiveShootCommand(BmClient *client, bmsg::CL_tank_shoot /*shootCmd*/)
    {
        auto it = tanks_.find(client);
        if (it == tanks_.end()) return;
        it->second.ctl.shoot(m_tick);
    }

    size_t count(BmClient *client) const
    {
        return tanks_.count(client);
    }

    void spawnTank(BmClient *client)
    {
        if (tanks_.count(client)) {
            std::cerr << "tank with client `" << client->id() << "` was already spawned\n";
            return;
        }

        tanks_.try_emplace(client, map_, client, animator_, assets_);
    }

private:
    void sendState()
    {
        ++m_tick;
        const auto size = map_->getSize();

        for (auto &[client, tank] : tanks_) {
            const Vec2i pos = tank.ctl.pos();

            client->send(bmsg::SV_tank_at{pos.x, pos.y});
            client->send(bmsg::SV_tank_hp{tank.ctl.hp()});

            sendVisible(client, tank.ctl);

            client->send(bmsg::SV_tank_tick{});
        }

        timer_->setTimer(1, [this]() { sendState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }

    void sendVisible(BmClient *client, TankCtl &ctl)
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
                client->send(bmsg::SV_tank_wall{pos.x, pos.y});
            }

            for (const auto &[id, entity] : tile->getEntityList()) {
                (void)id;

                if (entity == nullptr || entity == ctl.tank()) {
                    continue;
                }

                    if (combat_grid::isRoot(entity)) {
                        client->send(bmsg::SV_tank_root{
                            pos.x,
                            pos.y,
                            static_cast<uint32_t>(entity->getID())
                        });
                        continue;
                    }

                if (combat_grid::isCombatant(entity)) {
                    client->send(bmsg::SV_tank_enemy{
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
