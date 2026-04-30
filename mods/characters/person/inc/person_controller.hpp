#pragma once
#include "person_gfx.hpp"

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

    Map *map_=nullptr;
    Person *person_=nullptr;
    uint64_t m_nextMoveTick = 0;
    uint64_t m_nextAttackTick = 0;
    bool m_actionDone = false;

public:
    PersonCtl() = default;

    PersonCtl(Map *map, PersonAssetPack *assetPack): 
        map_(map)
    {
        assert(map);

        auto sz = map_->size(); 
        assert(sz.x > 2 && sz.y > 2);
        person_ = map_->spawn<PersonGfx>
            (
            Vec2i {
                1 + rand() % (sz.x - 2),
                1 + rand() % (sz.y - 2)
            },
            assetPack
        );
    }

    void move(int dx, int dy, uint64_t curTick) {
        assert(person_);
        assert(map_);
    
        if (curTick < m_nextMoveTick) return;
        if (abs(dx) > 1 || abs(dy) > 1) return;

        Vec2i newPos = {person_->pos().x + dx, person_->pos().y + dy};
        if (map_->at(newPos)->type() == Tile::BasicType::Wall) return;

        person_->rotate(convertMoveToDir(dx, dy));
        person_->move(newPos);
        m_nextMoveTick = curTick + kMoveCdTicks;
    }

    void attack(size_t whom, uint64_t curTick) {
        assert(person_);
        assert(map_);

        if (curTick < m_nextAttackTick) return;
        auto u = map_->byId(whom);
        if (!u) return;

        if (abs(u->pos().x - person_->pos().x) > 1 || abs(u->pos().y - person_->pos().y) > 1)
            return;
        
        int dmg = kBaseAttackDamage;
        if (person_->hp() <= kLowHpThreshold) dmg += kBerserkBonusDamage;
        u->takeDamage(dmg);

        m_nextAttackTick = curTick + kAttackCdTicks;
    }

    void setActionDoneState(bool flag) {
        m_actionDone = flag;
    }

    void destroy() {
        assert(person_);
        person_->destroy();
    }

    Vec2i pos() const { 
        assert(person_);
        return person_->pos(); 
    }
    int32_t hp() const {
        assert(person_); 
        return person_->hp(); 
    }
    Person *person() { 
        return person_; 
    }
};
  