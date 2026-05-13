#pragma once

#include "hunter.hpp"
#include "combat_grid.hpp"

#include <cassert>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string_view>

class HunterCtl {
    static constexpr uint64_t kMoveCdTicks  = 1;
    static constexpr uint64_t kVolleyCdTicks = 2;
    static constexpr uint64_t kMarkCdTicks   = 3;
    static constexpr int      kVolleyDamage  = 8;
    static constexpr int      kMarkedBonus   = 8;
    static constexpr int      kSplashDamage  = 4;

    Level                  *map_    = nullptr;
    std::unique_ptr<Hunter> hunter_ = nullptr;
    uint64_t m_nextMoveTick  = 0;
    uint64_t m_nextVolleyTick = 0;
    uint64_t m_nextMarkTick   = 0;
    std::optional<modlib::Entity::ID> m_markedTarget;

public:
    HunterCtl() = default;

    HunterCtl(Level *map, BmClient *client)
        : map_(map)
    {
        assert(map_);

        const Vec2i pos = randomWalkablePosition();
        Tile *tile = map_->getTile(pos);
        assert(tile);

        hunter_ = std::make_unique<Hunter>(map_, tile, client);
        map_->newEntity(hunter_.get(), tile);
    }

    void move(int dx, int dy, uint64_t curTick)
    {
        assert(hunter_);
        assert(map_);

        if (curTick < m_nextMoveTick) {
            return;
        }

        if (std::abs(dx) + std::abs(dy) != 1) {
            return;
        }

        const Vec2i delta{dx, dy};
        const Vec2i newPos = hunter_->getPosition() + delta;

        if (!combat_grid::canEnter(map_, newPos, hunter_.get())) {
            return;
        }

        hunter_->rotate(hunterDirFromDelta(delta));
        hunter_->setPosition(newPos);
        m_nextMoveTick = curTick + kMoveCdTicks;
    }

    bool useAbility(std::string_view ability, size_t targetId, uint64_t curTick)
    {
        assert(hunter_);

        if (!hunter_->inventory().hasAbility(ability)) {
            return false;
        }

        if (ability == "volley") {
            return useVolley(targetId, curTick);
        }
        if (ability == "mark") {
            return useMark(targetId, curTick);
        }

        return false;
    }

    void destroy()
    {
        assert(hunter_);
        map_->removeEntity(hunter_->getID());
    }

    Vec2i pos() const
    {
        assert(hunter_);
        return hunter_->getPosition();
    }

    int32_t hp() const
    {
        assert(hunter_);
        return static_cast<int32_t>(hunter_->getCurrentHP());
    }

    Hunter *hunter()
    {
        return hunter_.get();
    }

    Level *map()
    {
        return map_;
    }

private:
    bool useVolley(size_t whom, uint64_t curTick)
    {
        assert(hunter_);
        assert(map_);

        if (curTick < m_nextVolleyTick) {
            return false;
        }

        auto *target = map_->getEntity(static_cast<modlib::Entity::ID>(whom));
        if (target == nullptr || target == hunter_.get()) {
            return false;
        }

        const Vec2i delta = target->getPosition() - hunter_->getPosition();
        if (!combat_grid::inArcherRange(hunter_->getPosition(), target->getPosition())) {
            return false;
        }

        auto *health = dynamic_cast<EC::Stats::Health *>(target);
        if (health == nullptr || health->getCurrentHP() <= 0) {
            return false;
        }

        hunter_->rotate(hunterDirFromDelta(delta));

        const modlib::Entity::ID id = target->getID();
        hunter_->EvAttack.emit(id);
        int damage = kVolleyDamage;
        if (m_markedTarget.has_value() && *m_markedTarget == id) {
            damage += kMarkedBonus;
            m_markedTarget.reset();
        }
        health->inflictDmg(damage);

        const Vec2i center = target->getPosition();
        const auto size = map_->getSize();
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                if (dx == 0 && dy == 0) {
                    continue;
                }
                const Vec2i at{center.x + dx, center.y + dy};
                if (at.x < 0 || at.y < 0 || at.x >= size.x || at.y >= size.y) {
                    continue;
                }
                Tile *tile = map_->getTile(at);
                if (tile == nullptr) {
                    continue;
                }
                for (const auto &[entityId, entity] : tile->getEntityList()) {
                    (void)entityId;
                    if (entity == nullptr || entity == hunter_.get()) {
                        continue;
                    }
                    if (!combat_grid::isCombatant(entity)) {
                        continue;
                    }
                    auto *nearHealth = dynamic_cast<EC::Stats::Health *>(entity);
                    if (nearHealth != nullptr && nearHealth->getCurrentHP() > 0) {
                        nearHealth->inflictDmg(kSplashDamage);
                    }
                }
            }
        }

        m_nextVolleyTick = curTick + kVolleyCdTicks + 1;
        return true;
    }

    bool useMark(size_t whom, uint64_t curTick)
    {
        assert(hunter_);
        assert(map_);

        if (curTick < m_nextMarkTick) {
            return false;
        }

        auto *target = map_->getEntity(static_cast<modlib::Entity::ID>(whom));
        if (target == nullptr || target == hunter_.get()) {
            return false;
        }
        if (!combat_grid::isCombatant(target)) {
            return false;
        }
        if (!combat_grid::inArcherRange(hunter_->getPosition(), target->getPosition())) {
            return false;
        }

        m_markedTarget = target->getID();
        m_nextMarkTick = curTick + kMarkCdTicks + 1;
        return true;
    }

    Vec2i randomWalkablePosition() const
    {
        const auto sz = map_->getSize();
        assert(sz.x > 0 && sz.y > 0);

        for (int attempt = 0; attempt < 256; ++attempt) {
            const Vec2i pos{std::rand() % sz.x, std::rand() % sz.y};
            if (combat_grid::canEnter(map_, pos, hunter_.get())) {
                return pos;
            }
        }

        for (int x = 0; x < sz.x; ++x) {
            for (int y = 0; y < sz.y; ++y) {
                const Vec2i pos{x, y};
                if (combat_grid::canEnter(map_, pos, hunter_.get())) {
                    return pos;
                }
            }
        }

        return {0, 0};
    }
};
