#pragma once

#include "BmServerModule.hpp"
#include "Inventory.hpp"
#include "Map.hpp"

#include <cstdlib>

using namespace modlib;

enum class RogueDir {
    down  = 0,
    up    = 1,
    left  = 2,
    right = 3,
};

inline RogueDir rogueDirFromDelta(Vec2i delta)
{
    if (std::abs(delta.x) > std::abs(delta.y)) {
        return delta.x < 0 ? RogueDir::left : RogueDir::right;
    }

    if (delta.y != 0) {
        return delta.y < 0 ? RogueDir::up : RogueDir::down;
    }

    return RogueDir::down;
}

class Rogue :
    virtual public modlib::Entity,
    virtual public EC::Stats::Health,
    virtual public EC::Stats::Attack
{
public:
    static constexpr int  MAX_HP     = 45;
    static constexpr int  CURRENT_HP = 45;
    static constexpr int  STRENGTH   = 10;
    static constexpr Type ROGUE_TYPE = "rogue";

private:
    Level   *map_ = nullptr;
    RogueDir dir_ = RogueDir::down;
    modlib::Inventory inventory_;

public:
    Rogue(Level *map, Tile *tile, modlib::BmClient *client)
        : Entity(ROGUE_TYPE, tile)
        , Health(CURRENT_HP, MAX_HP)
        , Attack(STRENGTH)
        , map_(map)
    {
        (void)client;

        inventory_.addItem(modlib::ItemDef(
            "knife",
            {
                modlib::AbilityDef("slice"),
            }
        ));
    }

    void rotate(RogueDir dir)
    {
        dir_ = dir;
    }

    RogueDir dir() const
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
