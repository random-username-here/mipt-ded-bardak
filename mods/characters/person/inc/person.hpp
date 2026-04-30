#pragma once

#include "Map.hpp"
#include <cstdlib>
#include <optional>
#include <cassert>

#include "BmServerModule.hpp"
#include "Map.hpp"
#include "Timer.hpp"
#include "binmsg.hpp"
#include "modlib_mod.hpp"
#include "modlib_manager.hpp"
#include "AssetManager.hpp"
#include "AssetConfig.hpp"
#include <cstdlib>
#include <iostream>
#include <optional>
#include "./person_proto.hpp"

using namespace modlib;

enum class RotationDir {
    up,
    down,
    left, 
    right,
};

class Person : public Unit {
    const int max_hp = 200;

    Map *m_map;
    Vec2i m_pos;
    uint64_t m_id;
    int m_hp;
    RotationDir m_dir = RotationDir::down;

public:
    Person(Map *map, Vec2i pos, size_t id)
        :m_map(map), m_pos(pos), m_id(id), m_hp(max_hp) {}

    Map *map() override { return m_map; }
    Tile *tile() override { return m_map->at(m_pos); }
    
    uint64_t id() override { return m_id; }
    uint64_t type() const override { return 0; }
    uint64_t teamId() const override { return 0; }
    AssetId assetId() const override { return 7; }

    int hp() const override { return m_hp; }
    int maxHp() const override { return max_hp; }

    void takeDamage(int d) override {
        m_hp -= d;
        if (m_hp <= 0) {
            m_hp = 0;
            destroy();
        }
    }

    void pickUp() override {}
    int weight() const override { return 1; }
    void setWeight(const int weight) override {}

    Vec2i pos() const override { return m_pos; }

    void move(Vec2i to) override {
        Unit::move(to);
        m_pos = to;
    }

    void destroy() override {
        Unit::destroy();
    }

    void rotate(RotationDir dir) {
        m_dir = dir;
    }

};

inline RotationDir convertMoveToDir(int dx, int dy) {
    if (dx == 0 && dy == 0) return RotationDir::down; 

    if (std::abs(dx) > std::abs(dy)) {
        return (dx > 0) ? RotationDir::right: RotationDir::left;
    } else {
        return (dy > 0) ? RotationDir::down : RotationDir::up;
    }
};

// rules
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

    PersonCtl(Map *map): 
        map_(map)
    {
        assert(map);

        auto sz = map_->size(); 
        assert(sz.x > 2 && sz.y > 2);
        person_ = map_->spawn<Person>
            (Vec2i {
                1 + rand() % (sz.x - 2),
                1 + rand() % (sz.y - 2)
            });
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
  