#pragma once

#include "bomber.hpp"
#include "combat_grid.hpp"

#include <cassert>
#include <cstdlib>
#include <memory>
#include <string_view>

class BomberCtl {
    static constexpr uint64_t kMoveCdTicks  = 1;
    static constexpr uint64_t kShootCdTicks = 2;
    static constexpr int      kShootDamage  = 10;

    Level                  *map_    = nullptr;
    std::unique_ptr<Bomber> bomber_ = nullptr;
    uint64_t m_nextMoveTick  = 0;
    uint64_t m_nextShootTick = 0;

public:
    BomberCtl() = default;

    BomberCtl(Level *map, BmClient *client)
        : map_(map)
    {
        assert(map_);

        const Vec2i pos = randomWalkablePosition();
        Tile *tile = map_->getTile(pos);
        assert(tile);

        bomber_ = std::make_unique<Bomber>(map_, tile, client);
        map_->newEntity(bomber_.get(), tile);
    }

    void move(int dx, int dy, uint64_t curTick)
    {
        assert(bomber_);
        assert(map_);

        if (curTick < m_nextMoveTick) {
            return;
        }

        if (std::abs(dx) + std::abs(dy) != 1) {
            return;
        }

        const Vec2i delta{dx, dy};
        const Vec2i newPos = bomber_->getPosition() + delta;

        if (!combat_grid::canEnter(map_, newPos, bomber_.get())) {
            return;
        }

        bomber_->rotate(bomberDirFromDelta(delta));
        bomber_->setPosition(newPos);
        m_nextMoveTick = curTick + kMoveCdTicks;
    }

    void destroy()
    {
        assert(bomber_);
        map_->removeEntity(bomber_->getID());
    }

    Vec2i pos() const
    {
        assert(bomber_);
        return bomber_->getPosition();
    }

    int32_t hp() const
    {
        assert(bomber_);
        return static_cast<int32_t>(bomber_->getCurrentHP());
    }

    Bomber *bomber()
    {
        return bomber_.get();
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
            if (combat_grid::canEnter(map_, pos, bomber_.get())) {
                return pos;
            }
        }

        for (int x = 0; x < sz.x; ++x) {
            for (int y = 0; y < sz.y; ++y) {
                const Vec2i pos{x, y};
                if (combat_grid::canEnter(map_, pos, bomber_.get())) {
                    return pos;
                }
            }
        }

        return {0, 0};
    }
};
