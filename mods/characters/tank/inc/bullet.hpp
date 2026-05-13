#pragma once

#include "BmServerModule.hpp"
#include "Map.hpp"
#include "tank.hpp"

#include <cstdint>

using namespace modlib;

class Bullet :
    virtual public modlib::Entity,
    virtual public EC::Stats::Health,
    virtual public EC::Stats::Attack
{
public:
    static constexpr int MAX_HP     = 1;
    static constexpr int CURRENT_HP = 1;
    static constexpr int STRENGTH   = 10;
    static constexpr Type BULLET_TYPE = "bullet";

private:
    Level *map_ = nullptr;
    TankDir dir_ = TankDir::right;
    modlib::Entity::ID owner_ = 0;

public:
    Bullet(Level *map, Tile *tile, modlib::Entity::ID owner, TankDir dir)
        : Entity(BULLET_TYPE, tile)
        , Health(CURRENT_HP, MAX_HP)
        , Attack(STRENGTH)
        , map_(map)
        , dir_(dir)
        , owner_(owner)
    {}

    TankDir dir() const
    {
        return dir_;
    }

    Level *map()
    {
        return map_;
    }

    modlib::Entity::ID owner() const
    {
        return owner_;
    }
};
