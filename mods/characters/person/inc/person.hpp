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

class Person : 
    virtual public modlib::Entity, 
    virtual public EC::Stats::Health,
    virtual public EC::Stats::Attack    
{
public:
    static constexpr int MAX_HP = 40;
    static constexpr int CURRENT_HP = 100;
    static constexpr int STRENGTH = 10;
    static constexpr Type PERSON_TYPE = "person";
private:
    Level *map_;
    RotationDir dir_ = RotationDir::down;
public:

    Person(Level *map, Tile *tile, modlib::BmClient* client):
        Entity(PERSON_TYPE, tile),
        Health(MAX_HP, CURRENT_HP),
        Attack(STRENGTH),
        map_(map) {}

    void rotate(RotationDir dir) {
        dir_ = dir;
    }

    RotationDir dir() const { return dir_; }
};

