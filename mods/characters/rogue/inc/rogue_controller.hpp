#pragma once

#include "combat_grid.hpp"
#include "rogue.hpp"

#include <cassert>
#include <cstdlib>
#include <memory>
#include <string_view>

class RogueCtl {
    static constexpr uint64_t kMoveCdTicks  = 1;
    static constexpr uint64_t kSliceCdTicks = 0;
    static constexpr int      kSliceDamage  = 10;

    Level                 *map_   = nullptr;
    std::unique_ptr<Rogue> rogue_ = nullptr;
    uint64_t m_nextMoveTick = 0;
    uint64_t m_nextSliceTick = 0;

public:
    RogueCtl() = default;

    RogueCtl(Level *map, BmClient *client)
        : map_(map)
    {
        assert(map_);

        const Vec2i pos = randomWalkablePosition();
        Tile *tile = map_->getTile(pos);
        assert(tile);

        rogue_ = std::make_unique<Rogue>(map_, tile, client);
        map_->newEntity(rogue_.get(), tile);
    }

    void move(int dx, int dy, uint64_t curTick)
    {
        assert(rogue_);
        assert(map_);

        if (curTick < m_nextMoveTick) {
            return;
        }

        if (std::abs(dx) + std::abs(dy) != 1) {
            return;
        }

        const Vec2i delta{dx, dy};
        const Vec2i newPos = rogue_->getPosition() + delta;

        if (!combat_grid::canEnter(map_, newPos, rogue_.get())) {
            return;
        }

        rogue_->rotate(rogueDirFromDelta(delta));
        rogue_->setPosition(newPos);
        m_nextMoveTick = curTick + kMoveCdTicks;
    }

    bool useAbility(std::string_view ability, size_t targetId, uint64_t curTick)
    {
        assert(rogue_);

        if (!rogue_->inventory().hasAbility(ability)) {
            return false;
        }

        if (ability == "slice") {
            return useSlice(targetId, curTick);
        }

        return false;
    }

    void destroy()
    {
        assert(rogue_);
        map_->removeEntity(rogue_->getID());
    }

    Vec2i pos() const
    {
        assert(rogue_);
        return rogue_->getPosition();
    }

    int32_t hp() const
    {
        assert(rogue_);
        return static_cast<int32_t>(rogue_->getCurrentHP());
    }

    Rogue *rogue()
    {
        return rogue_.get();
    }

    Level *map()
    {
        return map_;
    }

private:
    bool useSlice(size_t whom, uint64_t curTick)
    {
        assert(rogue_);
        assert(map_);

        if (curTick < m_nextSliceTick) {
            return false;
        }

        auto *target = map_->getEntity(static_cast<modlib::Entity::ID>(whom));
        if (target == nullptr || target == rogue_.get()) {
            return false;
        }

        const Vec2i delta = target->getPosition() - rogue_->getPosition();
        if (!combat_grid::inVonNeumannRange(rogue_->getPosition(), target->getPosition())) {
            return false;
        }

        rogue_->rotate(rogueDirFromDelta(delta));

        if (auto *health = dynamic_cast<EC::Stats::Health *>(target)) {
            const modlib::Entity::ID id = target->getID();
            health->inflictDmg(kSliceDamage);
            rogue_->EvAttack.emit(id);
        }

        m_nextSliceTick = curTick + kSliceCdTicks;
        return true;
    }

    Vec2i randomWalkablePosition() const
    {
        const auto sz = map_->getSize();
        assert(sz.x > 0 && sz.y > 0);

        for (int attempt = 0; attempt < 256; ++attempt) {
            const Vec2i pos{std::rand() % sz.x, std::rand() % sz.y};
            if (combat_grid::canEnter(map_, pos, rogue_.get())) {
                return pos;
            }
        }

        for (int x = 0; x < sz.x; ++x) {
            for (int y = 0; y < sz.y; ++y) {
                const Vec2i pos{x, y};
                if (combat_grid::canEnter(map_, pos, rogue_.get())) {
                    return pos;
                }
            }
        }

        return {0, 0};
    }
};
