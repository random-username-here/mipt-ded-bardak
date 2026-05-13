#pragma once

#include "BmServerModule.hpp"
#include "Timer.hpp"
#include "binmsg.hpp"
#include "knight_animator.hpp"
#include "knight_controller.hpp"
#include "knight_proto.hpp"

#include <iostream>
#include <unordered_map>

class KnightManager {
    Timer *timer_ = nullptr;
    Level *map_   = nullptr;
    anim::AnimationManager *animator_ = nullptr;
    modlib::AssetManager   *assets_   = nullptr;

    struct KnightUtils {
        KnightCtl ctl;
        KnightAnimator anim;

        KnightUtils(
            Level    *map,
            BmClient *client,
            anim::AnimationManager *animator,
            modlib::AssetManager   *assets
        )
            : ctl(map, client)
            , anim(&ctl, animator, assets)
        {}
    };

    std::unordered_map<BmClient *, KnightUtils> knights_;
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
        auto it = knights_.find(client);
        if (it != knights_.end()) {
            it->second.ctl.destroy();
            knights_.erase(it);
            client->send(bmsg::SV_knight_hp{0});
        }
    }

    void resolve()
    {
        timer_->setTimer(1, [this]() { sendState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }

    void receiveMoveCommand(BmClient *client, bmsg::CL_knight_move moveCmd)
    {
        auto it = knights_.find(client);
        if (it == knights_.end()) {
            return;
        }

        it->second.ctl.move(moveCmd.dx, moveCmd.dy, m_tick);
    }

    void receiveUseCommand(BmClient *client, bmsg::CL_knight_use useCmd)
    {
        auto it = knights_.find(client);
        if (it == knights_.end()) {
            return;
        }

        it->second.ctl.useAbility(useCmd.ability, useCmd.target, m_tick);
    }

    size_t count(BmClient *client) const
    {
        return knights_.count(client);
    }

    void spawnKnight(BmClient *client)
    {
        if (knights_.count(client)) {
            std::cerr << "knight with client `" << client->id() << "` was already spawned\n";
            return;
        }

        knights_.try_emplace(client, map_, client, animator_, assets_);
    }

private:
    void sendState()
    {
        ++m_tick;
        const auto size = map_->getSize();

        for (auto &[client, knight] : knights_) {
            const Vec2i pos = knight.ctl.pos();

            client->send(bmsg::SV_knight_at{pos.x, pos.y});
            client->send(bmsg::SV_knight_hp{knight.ctl.hp()});

            sendRoots(client);
            sendWalls(client, size);
            sendInventory(client, knight.ctl.knight()->inventory());

            client->send(bmsg::SV_knight_tick{});
        }

        timer_->setTimer(1, [this]() { sendState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }

    void sendInventory(BmClient *client, const modlib::Inventory &inventory)
    {
        for (const auto &item : inventory.items()) {
            client->send(bmsg::SV_knight_item{item.id});
        }

        for (const auto &ability : inventory.abilities()) {
            client->send(bmsg::SV_knight_ability{ability.id});
        }
    }

    void sendRoots(BmClient *client)
    {
        for (const auto &[id, entity] : map_->getEntityList()) {
            (void)id;
            if (entity == nullptr || entity->getType() != modlib::Entity::BasicTypes::ROOT) {
                continue;
            }
            const Vec2i rootPos = entity->getPosition();
            client->send(bmsg::SV_knight_root{
                rootPos.x,
                rootPos.y,
                static_cast<uint32_t>(entity->getID())
            });
        }
    }

    void sendWalls(BmClient *client, Vec2i size)
    {
        for (int x = 0; x < size.x; ++x) {
            for (int y = 0; y < size.y; ++y) {
                Tile *tile = map_->getTile({x, y});
                if (tile != nullptr && tile->getType() == Tile::BasicTypes::WALL) {
                    client->send(bmsg::SV_knight_wall{x, y});
                }
            }
        }
    }
};
