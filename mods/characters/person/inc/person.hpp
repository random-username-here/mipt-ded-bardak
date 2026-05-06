#pragma once

#include "Map.hpp"
#include "person_base.hpp"

using namespace modlib;

enum class RotationDir {
    up,
    down,
    left,
    right,
};

class Person : public PersonBase {
public:
    static constexpr int ASSET_ID_STUB = 0;
    static constexpr int ARMOR = 0;
    static constexpr int RESIST = 0;
    static constexpr int MAX_HP = 200;
    static constexpr int CURRENT_HP = 200;
    static constexpr int STRENGTH = 10;
    static constexpr int TEAM_ID = 0;
    static constexpr Type PERSON_TYPE = "person";
private:
    Level *map_;
    RotationDir dir_ = RotationDir::down;
public:

    Person(Level *map, Tile *tile, modlib::BmClient* client):
        PersonBase(PERSON_TYPE, tile, {ARMOR, RESIST}, {MAX_HP, CURRENT_HP}, {STRENGTH}, {TEAM_ID},  client),
        map_(map) {}

    // AssetId assetId() const override { return ASSET_ID_STUB;}

    void rotate(RotationDir dir) {
        dir_ = dir;
    }

    RotationDir dir() const { return dir_; }
};

