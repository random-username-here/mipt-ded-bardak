#pragma once

#include "person.hpp"

inline RotationDir convertMoveToDir(int dx, int dy) {
    if (dx == 0 && dy == 0) return RotationDir::down;

    if (std::abs(dx) > std::abs(dy)) {
        return (dx > 0) ? RotationDir::right: RotationDir::left;
    } else {
        return (dy > 0) ? RotationDir::down : RotationDir::up;
    }
};

// Rules
class PersonCtl {
    static constexpr uint64_t kMoveCdTicks = 1;
    static constexpr uint64_t kAttackCdTicks = 2;
    static constexpr int kBaseAttackDamage = 10;
    static constexpr int kBerserkBonusDamage = 6;
    static constexpr int kLowHpThreshold = 25;

    Level *map_=nullptr;
    std::unique_ptr<Person> person_=nullptr;
    uint64_t m_nextMoveTick = 0;
    uint64_t m_nextAttackTick = 0;
    bool m_actionDone = false;

public:
    PersonCtl() = default;

    PersonCtl(Level *map, BmClient *client):
        map_(map)
    {
        assert(map);

        auto sz = map_->getSize();
        assert(sz.x > 2 && sz.y > 2);

        Vec2i pos = Vec2i
        {
            1 + rand() % (sz.x - 2),
            1 + rand() % (sz.y - 2)
        };

        Tile *tile = map->getTile(pos);
        person_ = std::make_unique<Person>(map, tile, client);
        map_->newEntity(person_.get(), tile);
    }

    void move(int dx, int dy, uint64_t curTick) {
        assert(person_);
        assert(map_);

        if (curTick < m_nextMoveTick) return;
        if (abs(dx) > 1 || abs(dy) > 1) return;

        Vec2i newPos = {person_->getPosition().x + dx, person_->getPosition().y + dy};
        if (map_->getTile(newPos)->getType() == Tile::BasicTypes::WALL) return;

        // person_->rotate(convertMoveToDir(dx, dy));
        person_->setPosition(newPos);
        m_nextMoveTick = curTick + kMoveCdTicks;
    }

    void attack(size_t whom, uint64_t curTick) {
        assert(person_);
        assert(map_);

        if (curTick < m_nextAttackTick) return;
        auto u = map_->getEntity(whom);
        if (!u) return;

        if (std::abs(u->getPosition().x - person_->getPosition().x) > 1 || std::abs(u->getPosition().y - person_->getPosition().y) > 1)
            return;

        int dmg = kBaseAttackDamage;
        if (person_->getCurrentHP() <= kLowHpThreshold) dmg += kBerserkBonusDamage;
        if (auto *entity = dynamic_cast<EC::Stats::Health *>(u)) {
            entity->inflictDmg(dmg);
            person_.get()->EvAttack.emit(u->getID());
        }

        m_nextAttackTick = curTick + kAttackCdTicks;
    }

    void setActionDoneState(bool flag) {
        m_actionDone = flag;
    }

    void destroy() { // FIXME!!! what if person_ is not destructed, but map deletes person
        assert(person_);
        map_->removeEntity(person_->getID());
    }

    Vec2i pos() const {
        assert(person_);
        return person_->getPosition();
    }
    int32_t hp() const {
        assert(person_);
        return person_->getCurrentHP();
    }
    Person *person() {
        return person_.get();
    }

    Level *map() {
        return map_;
    }
};
