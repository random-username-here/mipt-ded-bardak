#pragma once

#include "Map.hpp"

using namespace modlib;

enum class RotationDir {
    up,
    down,
    left, 
    right,
};

// Container 
class Person : public Unit {
public:
    static const int MAX_HP = 200;
    static const int TYPE = 0;
    static const int TEAM_ID = 0;
private:
    Map *map_;
    Vec2i pos_;
    uint64_t id_;
    int hp_=MAX_HP;
    RotationDir dir_ = RotationDir::down;
public:
    Person(Map *map, Vec2i pos, size_t id)
        :map_(map), pos_(pos), id_(id) {}

    Map *map() override { return map_; }
    Tile *tile() override { return map_->at(pos_); }
    
    uint64_t id() override { return id_; }
    uint64_t type() const override { return TYPE; }
    uint64_t teamId() const override { return TEAM_ID; }

    int hp() const override { return hp_; }
    int maxHp() const override { return MAX_HP; }

    void takeDamage(int d) override {
        hp_ -= d;
        if (hp_ <= 0) {
            hp_ = 0;
            destroy();
        }
    }

    void pickUp() override {}
    int weight() const override { return 1; }
    void setWeight(const int weight) override {}

    Vec2i pos() const override { return pos_; }

    void move(Vec2i to) override {
        Unit::move(to);
        pos_ = to;
    }

    void destroy() override {
        Unit::destroy();
    }

    void rotate(RotationDir dir) {
        dir_ = dir;
    }

    RotationDir dir() const { return dir_; }
};

