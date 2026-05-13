#pragma once

#include "BmServerModule.hpp"
#include "Inventory.hpp"
#include "Map.hpp"

#include <cstdlib>

using namespace modlib;

enum class PaladinDir {
    down  = 0,
    up    = 1,
    left  = 2,
    right = 3,
};

inline PaladinDir paladinDirFromDelta(Vec2i delta)
{
    if (std::abs(delta.x) > std::abs(delta.y)) {
        return delta.x < 0 ? PaladinDir::left : PaladinDir::right;
    }
    if (delta.y != 0) {
        return delta.y < 0 ? PaladinDir::up : PaladinDir::down;
    }
    return PaladinDir::down;
}

class Paladin :
    virtual public modlib::Entity,
    virtual public EC::Stats::Health,
    virtual public EC::Stats::Attack
{
public:
    static constexpr int MAX_HP     = 120;
    static constexpr int CURRENT_HP = 120;
    static constexpr int STRENGTH   = 12;
    static constexpr Type PALADIN_TYPE = "paladin";

private:
    Level    *map_ = nullptr;
    PaladinDir dir_ = PaladinDir::down;
    modlib::Inventory inventory_;

public:
    Paladin(Level *map, Tile *tile, modlib::BmClient *client)
        : Entity(PALADIN_TYPE, tile)
        , Health(CURRENT_HP, MAX_HP)
        , Attack(STRENGTH)
        , map_(map)
    {
        (void)client;

        inventory_.addItem(modlib::ItemDef(
            "warhammer",
            {
                modlib::AbilityDef("smite"),
                modlib::AbilityDef("aegis"),
            }
        ));
    }

    void rotate(PaladinDir dir)
    {
        dir_ = dir;
    }

    PaladinDir dir() const
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
