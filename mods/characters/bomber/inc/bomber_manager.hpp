#pragma once

#include "AssetManager.hpp"
#include "BmServerModule.hpp"
#include "Timer.hpp"
#include "bomber_animator.hpp"
#include "bomber_controller.hpp"
#include "bomber_proto.hpp"
#include "binmsg.hpp"
#include "combat_grid.hpp"

#include "bomb/bomb.hpp"

#include <iostream>
#include <unordered_map>

class BomberManager {
    Timer *timer_ = nullptr;
    Level *map_   = nullptr;
    anim::AnimationManager *animator_ = nullptr;
    modlib::AssetManager   *assets_   = nullptr;

    struct BomberUtils {
        BomberCtl ctl;
        BomberAnimator anim;

        BomberUtils(
            Level    *map,
            BmClient *client,
            anim::AnimationManager *animator,
            modlib::AssetManager   *assets
        )
            : ctl(map, client)
            , anim(&ctl, animator, assets)
        {}
    };

    std::unordered_map<BmClient *, BomberUtils> bombers_;
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
        auto it = bombers_.find(client);
        if (it != bombers_.end()) {
            it->second.ctl.destroy();
            bombers_.erase(it);
            client->send(bmsg::SV_bomber_hp{0});
        }
    }

    void resolve()
    {
        timer_->setTimer(1, [this]() { sendState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }

    void receiveMoveCommand(BmClient *client, bmsg::CL_bomber_move moveCmd)
    {
        auto it = bombers_.find(client);
        if (it == bombers_.end()) {
            return;
        }

        it->second.ctl.move(moveCmd.dx, moveCmd.dy, m_tick);
    }

    void receiveBombCommand(BmClient *client, bmsg::CL_bomber_bomb bombCmd, bombs::BombsModule *bombs_module)
    {
        auto it = bombers_.find(client);
        if (it == bombers_.end()) {
            return;
        }

        auto pos = it->second.ctl.pos();
        auto dir = it->second.ctl.bomber()->dir();
        switch (dir) {
            case BomberDir::down :
                pos.y--;
                break;
            case BomberDir::left :
                pos.x--;
                break;
            case BomberDir::right :
                pos.x++;
                break;
            case BomberDir::up :
                pos.y++;
                break;
        }
        bombs_module->spawnBomb(pos, it->second.ctl.bomber()->getID());
    }

    size_t count(BmClient *client) const
    {
        return bombers_.count(client);
    }

    void spawnBomber(BmClient *client)
    {
        if (bombers_.count(client)) {
            std::cerr << "bomber with client `" << client->id() << "` was already spawned\n";
            return;
        }

        bombers_.try_emplace(client, map_, client, animator_, assets_);
    }

private:
    void sendState()
    {
        ++m_tick;

        for (auto &[client, bomber] : bombers_) {
            const Vec2i pos = bomber.ctl.pos();

            client->send(bmsg::SV_bomber_at{pos.x, pos.y});
            client->send(bmsg::SV_bomber_hp{bomber.ctl.hp()});

            sendVisible  (client, bomber.ctl);
            sendInventory(client, bomber.ctl.bomber()->inventory());

            client->send(bmsg::SV_bomber_tick{});
        }

        timer_->setTimer(1, [this]() { sendState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }

    void sendVisible(BmClient *client, BomberCtl &ctl)
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
                client->send(bmsg::SV_bomber_wall{pos.x, pos.y});
            }

            for (const auto &[id, entity] : tile->getEntityList()) {
                (void)id;

                if (entity == nullptr || entity == ctl.bomber()) {
                    continue;
                }

                if (combat_grid::isRoot(entity)) {
                    client->send(bmsg::SV_bomber_root{
                        pos.x,
                        pos.y,
                        static_cast<uint32_t>(entity->getID())
                    });
                    continue;
                }

                if (combat_grid::isCombatant(entity)) {
                    client->send(bmsg::SV_bomber_enemy{
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
            client->send(bmsg::SV_bomber_item{item.id});
        }

        for (const auto &ability : inventory.abilities()) {
            client->send(bmsg::SV_bomber_ability{ability.id});
        }
    }
};
