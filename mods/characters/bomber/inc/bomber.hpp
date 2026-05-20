#pragma once

#include "BmServerModule.hpp"
#include "Inventory.hpp"
#include "Map.hpp"

#include <cstdlib>

using namespace modlib;

enum class BomberDir {
    down  = 0,
    up    = 1,
    left  = 2,
    right = 3,
};

inline BomberDir bomberDirFromDelta(Vec2i delta)
{
    if (std::abs(delta.x) > std::abs(delta.y)) {
        return delta.x < 0 ? BomberDir::left : BomberDir::right;
    }

    if (delta.y != 0) {
        return delta.y < 0 ? BomberDir::up : BomberDir::down;
    }

    return BomberDir::down;
}

class Bomber :
    virtual public modlib::Entity,
    virtual public EC::Stats::Health,
    virtual public EC::Stats::Attack
{
public:
    static constexpr int  MAX_HP      = 60;
    static constexpr int  CURRENT_HP  = 60;
    static constexpr int  STRENGTH    = 10;
    static constexpr Type BOMBER_TYPE = "bomber";

private:
    Level *map_ = nullptr;
    BomberDir dir_ = BomberDir::down;
    modlib::Inventory inventory_;

public:
    Bomber(Level *map, Tile *tile, modlib::BmClient *client)
        : Entity(BOMBER_TYPE, tile)
        , Health(CURRENT_HP, MAX_HP)
        , Attack(STRENGTH)
        , map_(map)
    {
        (void)client;
    }

    void rotate(BomberDir dir)
    {
        dir_ = dir;
    }

    BomberDir dir() const
    {
        return dir_;
    }

    Level *map()
    {
        return map_;
    }

    modlib::Inventory &inventory()
    {
        return inventory_;
    }

    const modlib::Inventory &inventory() const
    {
        return inventory_;
    }
};
