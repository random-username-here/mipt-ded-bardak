#pragma once

#include "knight.hpp"
#include "combat_grid.hpp"

#include <cassert>
#include <cstdlib>
#include <memory>
#include <string_view>

class KnightCtl {
    static constexpr uint64_t kMoveCdTicks   = 1;
    static constexpr uint64_t kSlashCdTicks  = 1;
    static constexpr int      kAttackDamage  = 10;

    Level *map_ = nullptr;
    std::unique_ptr<Knight> knight_ = nullptr;
    uint64_t m_nextMoveTick   = 0;
    uint64_t m_nextAttackTick = 0;

public:
    KnightCtl() = default;

    KnightCtl(Level *map, BmClient *client)
        : map_(map)
    {
        assert(map_);

        const Vec2i pos = randomWalkablePosition();
        Tile *tile = map_->getTile(pos);
        assert(tile);

        knight_ = std::make_unique<Knight>(map_, tile, client);
        map_->newEntity(knight_.get(), tile);
    }

    void move(int dx, int dy, uint64_t curTick)
    {
        assert(knight_);
        assert(map_);

        if (curTick < m_nextMoveTick) {
            return;
        }
        if (std::abs(dx) + std::abs(dy) != 1) {
            return;
        }

        const Vec2i delta{dx, dy};
        const Vec2i newPos = knight_->getPosition() + delta;
        if (!combat_grid::canEnter(map_, newPos, knight_.get())) {
            return;
        }

        knight_->rotate(knightDirFromDelta(delta));
        knight_->setPosition(newPos);
        m_nextMoveTick = curTick + kMoveCdTicks;
    }

    bool useAbility(std::string_view ability, size_t targetId, uint64_t curTick)
    {
        assert(knight_);

        if (!knight_->inventory().hasAbility(ability)) {
            return false;
        }

        if (ability == "slash") {
            return useSlash(targetId, curTick);
        }

        return false;
    }

    void destroy()
    {
        assert(knight_);
        map_->removeEntity(knight_->getID());
    }

    Vec2i pos() const
    {
        assert(knight_);
        return knight_->getPosition();
    }

    int32_t hp() const
    {
        assert(knight_);
        return static_cast<int32_t>(knight_->getCurrentHP());
    }

    Knight *knight()
    {
        return knight_.get();
    }

    Level *map()
    {
        return map_;
    }

private:
    bool useSlash(size_t whom, uint64_t curTick)
    {
        assert(knight_);
        assert(map_);

        if (curTick < m_nextAttackTick) {
            return false;
        }

        auto *target = map_->getEntity(static_cast<modlib::Entity::ID>(whom));
        if (target == nullptr || target == knight_.get()) {
            return false;
        }

        const Vec2i delta = target->getPosition() - knight_->getPosition();
        if (!combat_grid::inMooreRange(knight_->getPosition(), target->getPosition())) {
            return false;
        }

        auto *health = dynamic_cast<EC::Stats::Health *>(target);
        if (health == nullptr || health->getCurrentHP() <= 0) {
            return false;
        }

        knight_->rotate(knightDirFromDelta(delta));

        const modlib::Entity::ID id = target->getID();
        knight_->EvAttack.emit(id);
        health->inflictDmg(kAttackDamage);

        m_nextAttackTick = curTick + kSlashCdTicks + 1;
        return true;
    }

    Vec2i randomWalkablePosition() const
    {
        const auto sz = map_->getSize();
        assert(sz.x > 0 && sz.y > 0);

        for (int attempt = 0; attempt < 256; ++attempt) {
            const Vec2i pos{std::rand() % sz.x, std::rand() % sz.y};
            if (combat_grid::canEnter(map_, pos, knight_.get())) {
                return pos;
            }
        }

        for (int x = 0; x < sz.x; ++x) {
            for (int y = 0; y < sz.y; ++y) {
                const Vec2i pos{x, y};
                if (combat_grid::canEnter(map_, pos, knight_.get())) {
                    return pos;
                }
            }
        }

        return {0, 0};
    }
};
