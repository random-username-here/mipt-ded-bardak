#pragma once

#include "Map.hpp"

using namespace modlib;

enum class RotationDir {
    up,
    down,
    left, 
    right,
};

class Person : public virtual Entity, public virtual EC::Stats::Health {
public:
    static constexpr int ASSET_ID_STUB = 0;
    static constexpr int MAX_HP = 200;
    static constexpr int CURRENT_HP = 200;
    static constexpr int TEAM_ID = 0;
    static constexpr Type PERSON_TYPE = "person";
private:
    Level *map_;
    // RotationDir dir_ = RotationDir::down;
public:

    Person(Level *map, Tile *tile):
        EC::Entity(PERSON_TYPE),
        Entity(PERSON_TYPE, tile),
        Health(CURRENT_HP, MAX_HP),
        map_(map) {}

    Type getType() const override { return PERSON_TYPE; }


    // Level *map() override { return map_; }
    // Tile *tile() override { return map_->at(pos_); }
    
    // int hp() const override { return hp_; }
    // int maxHp() const override { return MAX_HP; }

    // void takeDamage(int d) override {
    //     hp_ -= d;
    //     if (hp_ <= 0) {
    //         hp_ = 0;
    //         destroy();
    //     }
    // }

    // void pickUp() override {}
    // int weight() const override { return 1; }
    // void setWeight(const int weight) override {}

    // Vec2i pos() const override { return pos_; }
    // AssetId assetId() const override { return ASSET_ID_STUB;}

    // void destroy() override {
    //     Unit::destroy();
    // }

    // void rotate(RotationDir dir) {
    //     dir_ = dir;
    // }

    // RotationDir dir() const { return dir_; }
};

