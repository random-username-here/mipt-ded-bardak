#pragma once

#include "BmServerModule.hpp"
#include "Inventory.hpp"
#include "Map.hpp"

#include <cstdlib>

using namespace modlib;

enum class ArcherDir {
    down  = 0,
    up    = 1,
    left  = 2,
    right = 3,
};

inline ArcherDir archerDirFromDelta(Vec2i delta)
{
    if (std::abs(delta.x) > std::abs(delta.y)) {
        return delta.x < 0 ? ArcherDir::left : ArcherDir::right;
    }

    if (delta.y != 0) {
        return delta.y < 0 ? ArcherDir::up : ArcherDir::down;
    }

    return ArcherDir::down;
}

class Archer :
    virtual public modlib::Entity,
    virtual public EC::Stats::Health,
    virtual public EC::Stats::Attack
{
public:
    static constexpr int  MAX_HP      = 60;
    static constexpr int  CURRENT_HP  = 60;
    static constexpr int  STRENGTH    = 10;
    static constexpr Type ARCHER_TYPE = "archer";

private:
    Level *map_ = nullptr;
    ArcherDir dir_ = ArcherDir::down;
    modlib::Inventory inventory_;

public:
    Archer(Level *map, Tile *tile, modlib::BmClient *client)
        : Entity(ARCHER_TYPE, tile)
        , Health(CURRENT_HP, MAX_HP)
        , Attack(STRENGTH)
        , map_(map)
    {
        (void)client;

        inventory_.addItem(modlib::ItemDef(
            "bow",
            {
                modlib::AbilityDef("shoot"),
            }
        ));
    }

    void rotate(ArcherDir dir)
    {
        dir_ = dir;
    }

    ArcherDir dir() const
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
