#pragma once

#include "BmServerModule.hpp"
#include "Inventory.hpp"
#include "Map.hpp"

#include <cstdlib>

using namespace modlib;

enum class HunterDir {
    down  = 0,
    up    = 1,
    left  = 2,
    right = 3,
};

inline HunterDir hunterDirFromDelta(Vec2i delta)
{
    if (std::abs(delta.x) > std::abs(delta.y)) {
        return delta.x < 0 ? HunterDir::left : HunterDir::right;
    }

    if (delta.y != 0) {
        return delta.y < 0 ? HunterDir::up : HunterDir::down;
    }

    return HunterDir::down;
}

class Hunter :
    virtual public modlib::Entity,
    virtual public EC::Stats::Health,
    virtual public EC::Stats::Attack
{
public:
    static constexpr int  MAX_HP      = 50;
    static constexpr int  CURRENT_HP  = 50;
    static constexpr int  STRENGTH    = 9;
    static constexpr Type HUNTER_TYPE = "hunter";

private:
    Level *map_ = nullptr;
    HunterDir dir_ = HunterDir::down;
    modlib::Inventory inventory_;

public:
    Hunter(Level *map, Tile *tile, modlib::BmClient *client)
        : Entity(HUNTER_TYPE, tile)
        , Health(CURRENT_HP, MAX_HP)
        , Attack(STRENGTH)
        , map_(map)
    {
        (void)client;

        inventory_.addItem(modlib::ItemDef(
            "crossbow",
            {
                modlib::AbilityDef("volley"),
                modlib::AbilityDef("mark"),
            }
        ));
    }

    void rotate(HunterDir dir)
    {
        dir_ = dir;
    }

    HunterDir dir() const
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
