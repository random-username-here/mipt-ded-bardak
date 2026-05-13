#pragma once

#include "paladin.hpp"
#include "combat_grid.hpp"

#include <cassert>
#include <cstdlib>
#include <memory>
#include <string_view>

inline bool inPaladinRange(modlib::Vec2i from, modlib::Vec2i to)
{
    const int dx = combat_grid::iabs(from.x - to.x);
    const int dy = combat_grid::iabs(from.y - to.y);

    if (dx == 0 && dy == 0) {
        return false;
    }

    /*
     * Literal 5x5 mask:
     *
     * 0 1 1 1 0
     * 1 1 1 1 1
     * 1 1 1 1 1
     * 1 1 1 1 1
     * 0 1 1 1 0
     */
    return dx <= 2 && dy <= 2 && !(dx == 2 && dy == 2);
}

class PaladinCtl {
    static constexpr uint64_t kMoveCdTicks   = 2;
    static constexpr uint64_t kSmiteCdTicks     = 3;
    static constexpr uint64_t kAegisCdTicks     = 8;
    static constexpr uint64_t kJudgementCdTicks = 4;
    static constexpr int      kSmiteDamage      = 10;
    static constexpr int      kJudgementDamage  = 8;
    static constexpr int      kJudgementVsRoot  = 14;
    static constexpr int      kAegisHeal        = 10;

    Level *map_ = nullptr;
    std::unique_ptr<Paladin> paladin_ = nullptr;
    uint64_t m_nextMoveTick   = 0;
    uint64_t m_nextSmiteTick  = 0;
    uint64_t m_nextAegisTick  = 0;
    uint64_t m_nextJudgementTick = 0;

public:
    PaladinCtl() = default;

    PaladinCtl(Level *map, BmClient *client)
        : map_(map)
    {
        assert(map_);

        const Vec2i pos = randomWalkablePosition();
        Tile *tile = map_->getTile(pos);
        assert(tile);

        paladin_ = std::make_unique<Paladin>(map_, tile, client);
        map_->newEntity(paladin_.get(), tile);
    }

    void move(int dx, int dy, uint64_t curTick)
    {
        assert(paladin_);
        assert(map_);

        if (curTick < m_nextMoveTick) {
            return;
        }
        if (std::abs(dx) + std::abs(dy) != 1) {
            return;
        }

        const Vec2i delta{dx, dy};
        const Vec2i newPos = paladin_->getPosition() + delta;
        if (!combat_grid::canEnter(map_, newPos, paladin_.get())) {
            return;
        }

        paladin_->rotate(paladinDirFromDelta(delta));
        paladin_->setPosition(newPos);
        m_nextMoveTick = curTick + kMoveCdTicks;
    }

    bool useAbility(std::string_view ability, size_t targetId, uint64_t curTick)
    {
        assert(paladin_);

        if (!paladin_->inventory().hasAbility(ability)) {
            return false;
        }

        if (ability == "smite") {
            return useSmite(targetId, curTick);
        }
        if (ability == "aegis") {
            return useAegis(curTick);
        }
        if (ability == "judgement") {
            return useJudgement(targetId, curTick);
        }

        return false;
    }

    void destroy()
    {
        assert(paladin_);
        map_->removeEntity(paladin_->getID());
    }

    Vec2i pos() const
    {
        assert(paladin_);
        return paladin_->getPosition();
    }

    int32_t hp() const
    {
        assert(paladin_);
        return static_cast<int32_t>(paladin_->getCurrentHP());
    }

    Paladin *paladin()
    {
        return paladin_.get();
    }

    Level *map()
    {
        return map_;
    }

private:
    bool useSmite(size_t whom, uint64_t curTick)
    {
        assert(paladin_);
        assert(map_);

        if (curTick < m_nextSmiteTick) {
            return false;
        }

        auto *target = map_->getEntity(static_cast<modlib::Entity::ID>(whom));
        if (target == nullptr || target == paladin_.get()) {
            return false;
        }

        const Vec2i delta = target->getPosition() - paladin_->getPosition();
        if (!combat_grid::inMooreRange(paladin_->getPosition(), target->getPosition())) {
            return false;
        }

        auto *health = dynamic_cast<EC::Stats::Health *>(target);
        if (health == nullptr || health->getCurrentHP() <= 0) {
            return false;
        }

        paladin_->rotate(paladinDirFromDelta(delta));

        const modlib::Entity::ID id = target->getID();
        paladin_->EvAttack.emit(id);
        health->inflictDmg(kSmiteDamage);

        m_nextSmiteTick = curTick + kSmiteCdTicks + 1;
        return true;
    }

    bool useAegis(uint64_t curTick)
    {
        assert(paladin_);

        if (curTick < m_nextAegisTick) {
            return false;
        }
        if (paladin_->getCurrentHP() >= paladin_->getMaxHP()) {
            return false;
        }

        paladin_->heal(kAegisHeal);
        m_nextAegisTick = curTick + kAegisCdTicks + 1;
        return true;
    }

    bool useJudgement(size_t whom, uint64_t curTick)
    {
        assert(paladin_);
        assert(map_);

        if (curTick < m_nextJudgementTick) {
            return false;
        }

        auto *target = map_->getEntity(static_cast<modlib::Entity::ID>(whom));
        if (target == nullptr || target == paladin_.get()) {
            return false;
        }

        const Vec2i delta = target->getPosition() - paladin_->getPosition();
        if (!inPaladinRange(paladin_->getPosition(), target->getPosition())) {
            return false;
        }

        auto *health = dynamic_cast<EC::Stats::Health *>(target);
        if (health == nullptr || health->getCurrentHP() <= 0) {
            return false;
        }

        paladin_->rotate(paladinDirFromDelta(delta));
        paladin_->EvAttack.emit(target->getID());

        const int damage = combat_grid::isRoot(target) ? kJudgementVsRoot : kJudgementDamage;
        health->inflictDmg(damage);

        m_nextJudgementTick = curTick + kJudgementCdTicks + 1;
        return true;
    }

    Vec2i randomWalkablePosition() const
    {
        const auto sz = map_->getSize();
        assert(sz.x > 0 && sz.y > 0);

        for (int attempt = 0; attempt < 256; ++attempt) {
            const Vec2i pos{std::rand() % sz.x, std::rand() % sz.y};
            if (combat_grid::canEnter(map_, pos, paladin_.get())) {
                return pos;
            }
        }

        for (int x = 0; x < sz.x; ++x) {
            for (int y = 0; y < sz.y; ++y) {
                const Vec2i pos{x, y};
                if (combat_grid::canEnter(map_, pos, paladin_.get())) {
                    return pos;
                }
            }
        }

        return {0, 0};
    }
};
