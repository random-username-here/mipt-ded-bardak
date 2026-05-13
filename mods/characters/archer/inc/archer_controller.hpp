#pragma once

#include "archer.hpp"
#include "combat_grid.hpp"

#include <cassert>
#include <cstdlib>
#include <memory>
#include <string_view>

class ArcherCtl {
    static constexpr uint64_t kMoveCdTicks  = 1;
    static constexpr uint64_t kShootCdTicks = 2;
    static constexpr int      kShootDamage  = 10;

    Level                  *map_    = nullptr;
    std::unique_ptr<Archer> archer_ = nullptr;
    uint64_t m_nextMoveTick  = 0;
    uint64_t m_nextShootTick = 0;

public:
    ArcherCtl() = default;

    ArcherCtl(Level *map, BmClient *client)
        : map_(map)
    {
        assert(map_);

        const Vec2i pos = randomWalkablePosition();
        Tile *tile = map_->getTile(pos);
        assert(tile);

        archer_ = std::make_unique<Archer>(map_, tile, client);
        map_->newEntity(archer_.get(), tile);
    }

    void move(int dx, int dy, uint64_t curTick)
    {
        assert(archer_);
        assert(map_);

        if (curTick < m_nextMoveTick) {
            return;
        }

        if (std::abs(dx) + std::abs(dy) != 1) {
            return;
        }

        const Vec2i delta{dx, dy};
        const Vec2i newPos = archer_->getPosition() + delta;

        if (!combat_grid::canEnter(map_, newPos, archer_.get())) {
            return;
        }

        archer_->rotate(archerDirFromDelta(delta));
        archer_->setPosition(newPos);
        m_nextMoveTick = curTick + kMoveCdTicks;
    }

    bool useAbility(std::string_view ability, size_t targetId, uint64_t curTick)
    {
        assert(archer_);

        if (!archer_->inventory().hasAbility(ability)) {
            return false;
        }

        if (ability == "shoot") {
            return useShoot(targetId, curTick);
        }

        return false;
    }

    void destroy()
    {
        assert(archer_);
        map_->removeEntity(archer_->getID());
    }

    Vec2i pos() const
    {
        assert(archer_);
        return archer_->getPosition();
    }

    int32_t hp() const
    {
        assert(archer_);
        return static_cast<int32_t>(archer_->getCurrentHP());
    }

    Archer *archer()
    {
        return archer_.get();
    }

    Level *map()
    {
        return map_;
    }

private:
    bool useShoot(size_t whom, uint64_t curTick)
    {
        assert(archer_);
        assert(map_);

        if (curTick < m_nextShootTick) {
            return false;
        }

        auto *target = map_->getEntity(static_cast<modlib::Entity::ID>(whom));
        if (target == nullptr || target == archer_.get()) {
            return false;
        }

        const Vec2i delta = target->getPosition() - archer_->getPosition();
        if (!combat_grid::inArcherRange(archer_->getPosition(), target->getPosition())) {
            return false;
        }

        archer_->rotate(archerDirFromDelta(delta));

        if (auto *health = dynamic_cast<EC::Stats::Health *>(target)) {
            const modlib::Entity::ID id = target->getID();
            health->inflictDmg(kShootDamage);
            archer_->EvAttack.emit(id);
        }

        m_nextShootTick = curTick + kShootCdTicks;
        return true;
    }

    Vec2i randomWalkablePosition() const
    {
        const auto sz = map_->getSize();
        assert(sz.x > 0 && sz.y > 0);

        for (int attempt = 0; attempt < 256; ++attempt) {
            const Vec2i pos{std::rand() % sz.x, std::rand() % sz.y};
            if (combat_grid::canEnter(map_, pos, archer_.get())) {
                return pos;
            }
        }

        for (int x = 0; x < sz.x; ++x) {
            for (int y = 0; y < sz.y; ++y) {
                const Vec2i pos{x, y};
                if (combat_grid::canEnter(map_, pos, archer_.get())) {
                    return pos;
                }
            }
        }

        return {0, 0};
    }
};
