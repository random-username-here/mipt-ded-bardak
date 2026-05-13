#pragma once

#include "tank.hpp"
#include "combat_grid.hpp"

#include <cassert>
#include <cstdlib>
#include <memory>
#include <string_view>

class TankCtl {
    static constexpr uint64_t kMoveCdTicks  = 1;
    static constexpr uint64_t kShootCdTicks = 2;
    static constexpr int      kShootDamage  = 10;

    Level *map_ = nullptr;
    std::unique_ptr<Tank> tank_ = nullptr;
    uint64_t m_nextMoveTick  = 0;
    uint64_t m_nextShootTick = 0;

public:
    TankCtl() = default;

    TankCtl(Level *map, BmClient *client)
        : map_(map)
    {
        assert(map_);

        const Vec2i pos = randomWalkablePosition();
        Tile *tile = map_->getTile(pos);
        assert(tile);

        tank_ = std::make_unique<Tank>(map_, tile, client);
        map_->newEntity(tank_.get(), tile);
    }

    void move(uint64_t curTick)
    {
        assert(tank_);
        assert(map_);

        if (tank_->getCurrentHP() <= 0) {
            return;
        }
        if (curTick < m_nextMoveTick) {
            return;
        }

        const Vec2i delta = tankDirDelta(tank_->dir());
        const Vec2i newPos = tank_->getPosition() + delta;

        if (!combat_grid::canEnter(map_, newPos, tank_.get())) {
            return;
        }

        tank_->setPosition(newPos);
        m_nextMoveTick = curTick + kMoveCdTicks;
    }

    void rotate(int8_t dir, uint64_t /*curTick*/)
    {
        assert(tank_);
        tank_->rotate(tankDirFromClient(dir));
    }

    bool shoot(uint64_t curTick)
    {
        assert(tank_);
        assert(map_);

        if (tank_->getCurrentHP() <= 0) {
            return false;
        }
        if (curTick < m_nextShootTick) {
            return false;
        }

        const Vec2i origin = tank_->getPosition();
        const Vec2i targetPos = origin + tankDirDelta(tank_->dir());

        Tile *tile = map_->getTile(targetPos);
        if (!tile) {
            return false;
        }

        for (const auto &[id, entity] : tile->getEntityList()) {
            (void)id;
            if (entity == nullptr || entity == tank_.get()) {
                continue;
            }
            if (!combat_grid::isCombatant(entity)) {
                continue;
            }

            auto *health = dynamic_cast<EC::Stats::Health *>(entity);
            if (health == nullptr || health->getCurrentHP() <= 0) {
                continue;
            }

            // Face target (in case the direction got out of sync).
            if (entity->getPosition().x < origin.x) {
                tank_->rotate(TankDir::left);
            } else if (entity->getPosition().x > origin.x) {
                tank_->rotate(TankDir::right);
            }

            const modlib::Entity::ID targetId = entity->getID();
            tank_->EvAttack.emit(targetId);
            health->inflictDmg(kShootDamage);

            m_nextShootTick = curTick + kShootCdTicks + 1;
            return true;
        }

        return false;
    }

    void destroy()
    {
        assert(tank_);
        map_->removeEntity(tank_->getID());
    }

    Vec2i pos() const
    {
        assert(tank_);
        return tank_->getPosition();
    }

    int32_t hp() const
    {
        assert(tank_);
        return static_cast<int32_t>(tank_->getCurrentHP());
    }

    Tank *tank()
    {
        return tank_.get();
    }

    Level *map()
    {
        return map_;
    }

private:
    Vec2i randomWalkablePosition() const
    {
        const auto sz = map_->getSize();
        assert(sz.x > 0 && sz.y > 0);

        for (int attempt = 0; attempt < 256; ++attempt) {
            const Vec2i pos{std::rand() % sz.x, std::rand() % sz.y};
            if (combat_grid::canEnter(map_, pos, tank_.get())) {
                return pos;
            }
        }

        for (int x = 0; x < sz.x; ++x) {
            for (int y = 0; y < sz.y; ++y) {
                const Vec2i pos{x, y};
                if (combat_grid::canEnter(map_, pos, tank_.get())) {
                    return pos;
                }
            }
        }

        return {0, 0};
    }
};
