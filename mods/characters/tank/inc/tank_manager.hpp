#pragma once

#include "BmServerModule.hpp"
#include "Timer.hpp"
#include "binmsg.hpp"
#include "bullet_animator.hpp"
#include "bullet_controller.hpp"
#include "combat_grid.hpp"
#include "tank_animator.hpp"
#include "tank_controller.hpp"
#include "tank_proto.hpp"

#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

class TankManager {
    Timer *timer_ = nullptr;
    Level *map_   = nullptr;
    anim::AnimationManager *animator_ = nullptr;
    modlib::AssetManager   *assets_   = nullptr;

    struct TankUtils {
        TankCtl ctl;
        TankAnimator anim;

        TankUtils(
            Level *map,
            BmClient *client,
            anim::AnimationManager *animator,
            modlib::AssetManager *assets
        )
            : ctl(map, client)
            , anim(&ctl, animator, assets)
        {}
    };

    struct BulletUtils {
        BulletCtl ctl;
        BulletAnimator anim;

        BulletUtils(
            Level *map,
            const TankCtl::ShotRequest &shot,
            anim::AnimationManager *animator,
            modlib::AssetManager *assets
        )
            : ctl(map, shot.pos, shot.dir, shot.ownerId, /*spawnTick*/0)
            , anim(&ctl, animator, assets)
        {}

        BulletUtils(
            Level *map,
            const TankCtl::ShotRequest &shot,
            uint64_t spawnTick,
            anim::AnimationManager *animator,
            modlib::AssetManager *assets
        )
            : ctl(map, shot.pos, shot.dir, shot.ownerId, spawnTick)
            , anim(&ctl, animator, assets)
        {}
    };

    std::unordered_map<BmClient *, TankUtils> tanks_;
    std::vector<std::unique_ptr<BulletUtils>> bullets_;
    uint64_t m_tick = 0;

public:
    void setModules(
        Timer *timer,
        Level *map,
        anim::AnimationManager *animator,
        modlib::AssetManager *assets
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
            const auto tankId = it->second.ctl.id();
            it->second.ctl.destroy();
            tanks_.erase(it);
            client->send(bmsg::SV_tank_hp{0});

            for (auto bulletIt = bullets_.begin(); bulletIt != bullets_.end(); ) {
                if ((*bulletIt)->ctl.owner() == tankId) {
                    (*bulletIt)->ctl.destroy();
                    bulletIt = bullets_.erase(bulletIt);
                } else {
                    ++bulletIt;
                }
            }
        }
    }

    void resolve()
    {
        timer_->setTimer(1, [this]() { sendState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }

    void receiveMoveCommand(BmClient *client, bmsg::CL_tank_move /*moveCmd*/)
    {
        auto it = tanks_.find(client);
        if (it == tanks_.end()) {
            return;
        }

        it->second.ctl.move(m_tick);
    }

    void receiveRotateCommand(BmClient *client, bmsg::CL_tank_rotate rotateCmd)
    {
        auto it = tanks_.find(client);
        if (it == tanks_.end()) {
            return;
        }

        it->second.ctl.rotate(rotateCmd.dir, m_tick);
    }

    void receiveShootCommand(BmClient *client, bmsg::CL_tank_shoot /*shootCmd*/)
    {
        auto it = tanks_.find(client);
        if (it == tanks_.end()) {
            return;
        }

        const auto shot = it->second.ctl.shoot(m_tick);
        if (shot) {
            spawnBullet(*shot, m_tick);
        }
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
    void spawnBullet(const TankCtl::ShotRequest &shot, uint64_t spawnTick)
    {
        bullets_.push_back(std::make_unique<BulletUtils>(map_, shot, spawnTick, animator_, assets_));
    }

    void updateBullets()
    {
        for (auto it = bullets_.begin(); it != bullets_.end(); ) {
            auto &bullet = *it;
            if (!bullet->ctl.step(m_tick) || bullet->ctl.hp() <= 0) {
                bullet->ctl.destroy();
                it = bullets_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void sendState()
    {
        ++m_tick;
        updateBullets();

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
