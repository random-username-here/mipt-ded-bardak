#pragma once

#include "BmServerModule.hpp"
#include "Event.hpp"
#include "Inventory.hpp"
#include "Map.hpp"

#include <cstdlib>

using namespace modlib;

enum class MageDir {
    down  = 0,
    up    = 1,
    left  = 2,
    right = 3,
};

inline MageDir mageDirFromDelta(Vec2i delta)
{
    if (std::abs(delta.x) > std::abs(delta.y)) {
        return delta.x < 0 ? MageDir::left : MageDir::right;
    }

    if (delta.y != 0) {
        return delta.y < 0 ? MageDir::up : MageDir::down;
    }

    return MageDir::down;
}

class Mage :
    virtual public modlib::Entity,
    virtual public EC::Stats::Health,
    virtual public EC::Stats::Attack
{
public:
    static constexpr int  MAX_HP      = 55;
    static constexpr int  CURRENT_HP  = 55;
    static constexpr int  STRENGTH    = 10;
    static constexpr Type MAGE_TYPE   = "mage";

private:
    Level  *map_ = nullptr;
    MageDir dir_ = MageDir::down;
    modlib::Inventory inventory_;

public:
    Event<bmsg::Char64, Vec2i> EvCast;
    Event<Vec2i> EvFlameTile;

    Mage(Level *map, Tile *tile, modlib::BmClient *client)
        : Entity(MAGE_TYPE, tile)
        , Health(CURRENT_HP, MAX_HP)
        , Attack(STRENGTH)
        , map_(map)
    {
        (void)client;

        inventory_.addItem(modlib::ItemDef("heal-scroll",  {modlib::AbilityDef("heal")}));
        inventory_.addItem(modlib::ItemDef("flame-scroll", {modlib::AbilityDef("flame")}));
        inventory_.addItem(modlib::ItemDef("plant-scroll", {modlib::AbilityDef("plant")}));
    }

    void rotate(MageDir dir)
    {
        dir_ = dir;
    }

    MageDir dir() const
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
