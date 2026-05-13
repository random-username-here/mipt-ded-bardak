#pragma once

#include "Roots.hpp"
#include "combat_grid.hpp"
#include "mage.hpp"

#include <array>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <string_view>

class MageCtl {
    static constexpr uint64_t kMoveCdTicks  = 1;
    static constexpr uint64_t kHealCdTicks  = 2;
    static constexpr uint64_t kFlameCdTicks = 2;
    static constexpr uint64_t kPlantCdTicks = 3;

    static constexpr int kHealAmount  = 15;
    static constexpr int kFlameDamage = 10;

    Level                *map_   = nullptr;
    modlib::RootSystem   *roots_ = nullptr;
    std::unique_ptr<Mage> mage_  = nullptr;

    uint64_t m_nextMoveTick  = 0;
    uint64_t m_nextHealTick  = 0;
    uint64_t m_nextFlameTick = 0;
    uint64_t m_nextPlantTick = 0;

public:
    MageCtl() = default;

    MageCtl(Level *map, modlib::RootSystem *roots, BmClient *client)
        : map_(map)
        , roots_(roots)
    {
        assert(map_);
        assert(roots_);

        const Vec2i pos = randomWalkablePosition();
        Tile *tile = map_->getTile(pos);
        assert(tile);

        mage_ = std::make_unique<Mage>(map_, tile, client);
        map_->newEntity(mage_.get(), tile);
    }

    void move(int dx, int dy, uint64_t curTick)
    {
        assert(mage_);
        assert(map_);

        if (mage_->getCurrentHP() <= 0) {
            return;
        }

        if (curTick < m_nextMoveTick) {
            return;
        }

        if (std::abs(dx) + std::abs(dy) != 1) {
            return;
        }

        const Vec2i delta{dx, dy};
        const Vec2i newPos = mage_->getPosition() + delta;

        if (!combat_grid::canEnter(map_, newPos, mage_.get())) {
            return;
        }

        mage_->rotate(mageDirFromDelta(delta));
        mage_->setPosition(newPos);
        m_nextMoveTick = curTick + kMoveCdTicks;
    }

    bool useAbility(std::string_view ability, size_t targetId, Vec2i point, uint64_t curTick)
    {
        assert(mage_);
        (void)targetId;

        if (mage_->getCurrentHP() <= 0) {
            return false;
        }

        if (!mage_->inventory().hasAbility(ability)) {
            return false;
        }

        if (ability == "heal") {
            return useHeal(curTick);
        }

        if (ability == "flame") {
            return useFlame(curTick);
        }

        if (ability == "plant") {
            return usePlant(point, curTick);
        }

        return false;
    }

    void destroy()
    {
        assert(mage_);
        map_->removeEntity(mage_->getID());
    }

    Vec2i pos() const
    {
        assert(mage_);
        return mage_->getPosition();
    }

    int32_t hp() const
    {
        assert(mage_);
        return static_cast<int32_t>(mage_->getCurrentHP());
    }

    Mage *mage()
    {
        return mage_.get();
    }

    Level *map()
    {
        return map_;
    }

private:
    bool useHeal(uint64_t curTick)
    {
        if (curTick < m_nextHealTick) {
            return false;
        }

        if (mage_->getCurrentHP() >= mage_->getMaxHP()) {
            return false;
        }

        mage_->EvCast.emit(bmsg::Char64("heal"), mage_->getPosition());
        mage_->heal(kHealAmount);

        m_nextHealTick = curTick + kHealCdTicks + 1;
        return true;
    }

    bool useFlame(uint64_t curTick)
    {
        if (curTick < m_nextFlameTick) {
            return false;
        }

        const Vec2i origin = mage_->getPosition();
        const Vec2i size = map_->getSize();

        mage_->EvCast.emit(bmsg::Char64("flame"), origin);

        for (int dx = -2; dx <= 2; ++dx) {
            for (int dy = -2; dy <= 2; ++dy) {
                const Vec2i pos{origin.x + dx, origin.y + dy};

                if (!combat_grid::inMageFlameRange(origin, pos)) {
                    continue;
                }

                if (pos.x < 0 || pos.y < 0 || pos.x >= size.x || pos.y >= size.y) {
                    continue;
                }

                mage_->EvFlameTile.emit(pos);

                Tile *tile = map_->getTile(pos);
                if (tile == nullptr) {
                    continue;
                }

                for (const auto &[id, entity] : tile->getEntityList()) {
                    (void)id;

                    if (entity == nullptr || entity == mage_.get()) {
                        continue;
                    }

                    if (!combat_grid::isCombatant(entity)) {
                        continue;
                    }

                    if (auto *health = dynamic_cast<EC::Stats::Health *>(entity)) {
                        health->inflictDmg(kFlameDamage);
                    }
                }
            }
        }

        m_nextFlameTick = curTick + kFlameCdTicks + 1;
        return true;
    }

    bool usePlant(Vec2i center, uint64_t curTick)
    {
        if (curTick < m_nextPlantTick) {
            return false;
        }

        if (!combat_grid::inArcherRange(mage_->getPosition(), center)) {
            return false;
        }

        static constexpr std::array<Vec2i, 4> offsets{{
            { 1,  0},
            {-1,  0},
            { 0,  1},
            { 0, -1},
        }};

        bool grewAny = false;

        for (const Vec2i off : offsets) {
            const Vec2i pos{center.x + off.x, center.y + off.y};
            const auto id = roots_->spawnRoot(pos, true);
            grewAny = grewAny || id != modlib::RootSystem::INVALID_ROOT_ID;
        }

        if (!grewAny) {
            return false;
        }

        mage_->EvCast.emit(bmsg::Char64("plant"), center);
        m_nextPlantTick = curTick + kPlantCdTicks + 1;
        return true;
    }

    Vec2i randomWalkablePosition() const
    {
        const auto sz = map_->getSize();
        assert(sz.x > 0 && sz.y > 0);

        for (int attempt = 0; attempt < 256; ++attempt) {
            const Vec2i pos{std::rand() % sz.x, std::rand() % sz.y};
            if (combat_grid::canEnter(map_, pos, mage_.get())) {
                return pos;
            }
        }

        for (int x = 0; x < sz.x; ++x) {
            for (int y = 0; y < sz.y; ++y) {
                const Vec2i pos{x, y};
                if (combat_grid::canEnter(map_, pos, mage_.get())) {
                    return pos;
                }
            }
        }

        return {0, 0};
    }
};
