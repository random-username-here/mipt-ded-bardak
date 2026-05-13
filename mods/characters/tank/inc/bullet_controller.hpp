#pragma once

#include "bullet.hpp"
#include "combat_grid.hpp"

#include <cassert>
#include <memory>

class BulletCtl {
    static constexpr int kBulletDamage = 10;

    Level *map_ = nullptr;
    std::unique_ptr<Bullet> bullet_ = nullptr;
    uint64_t m_spawnTick = 0;

public:
    BulletCtl() = default;

    BulletCtl(Level *map, Vec2i pos, TankDir dir, modlib::Entity::ID owner, uint64_t spawnTick)
        : map_(map)
        , m_spawnTick(spawnTick)
    {
        assert(map_);

        Tile *tile = map_->getTile(pos);
        assert(tile);

        bullet_ = std::make_unique<Bullet>(map_, tile, owner, dir);
        map_->newEntity(bullet_.get(), tile);
    }

    bool step(uint64_t curTick)
    {
        assert(bullet_);
        assert(map_);

        if (bullet_->getCurrentHP() <= 0) {
            return false;
        }

        if (curTick <= m_spawnTick) {
            return true;
        }

        const Vec2i current = bullet_->getPosition();
        const Vec2i nextPos = current + tankDirDelta(bullet_->dir());

        Tile *tile = map_->getTile(nextPos);
        if (!tile) {
            bullet_->inflictDmg(bullet_->getCurrentHP());
            return false;
        }

        if (tile->getType() == Tile::BasicTypes::WALL) {
            bullet_->inflictDmg(bullet_->getCurrentHP());
            return false;
        }

        for (const auto &[id, entity] : tile->getEntityList()) {
            (void)id;

            if (entity == nullptr || entity == bullet_.get() || entity->getID() == bullet_->owner()) {
                continue;
            }

            if (!combat_grid::isCombatant(entity)) {
                continue;
            }

            auto *health = dynamic_cast<EC::Stats::Health *>(entity);
            if (health == nullptr || health->getCurrentHP() <= 0) {
                continue;
            }

            health->inflictDmg(kBulletDamage);
            bullet_->inflictDmg(bullet_->getCurrentHP());
            return false;
        }

        bullet_->setPosition(nextPos);
        return bullet_->getCurrentHP() > 0;
    }

    void destroy()
    {
        if (bullet_) {
            map_->removeEntity(bullet_->getID());
        }
    }

    Vec2i pos() const
    {
        assert(bullet_);
        return bullet_->getPosition();
    }

    int32_t hp() const
    {
        assert(bullet_);
        return static_cast<int32_t>(bullet_->getCurrentHP());
    }

    modlib::Entity::ID id() const
    {
        assert(bullet_);
        return bullet_->getID();
    }

    modlib::Entity::ID owner() const
    {
        assert(bullet_);
        return bullet_->owner();
    }

    Bullet *bullet()
    {
        return bullet_.get();
    }
};
