#pragma once

#include "BmServerModule.hpp"
#include "ECbasis.hpp"
#include "Inventory.hpp"
#include "Map.hpp"

#include <cstdlib>

using namespace modlib;


enum class Dir 
{
    DOWN  = 0,
    UP    = 1,
    LEFT  = 2,
    RIGHT = 3,
};

inline
Dir
Delta2Dir (
    Vec2i delta
)
{
    if (std::abs (delta.x) > std::abs (delta.y))
    {
        return delta.x < 0 ? Dir::LEFT : Dir::RIGHT;
    }
    if (delta.y != 0)
    {
        return delta.y < 0 ? Dir::UP : Dir::DOWN;
    }
    return Dir::DOWN;
}


class Priest
: virtual public modlib::Entity
, virtual public EC::Stats::Health
, virtual public EC::Stats::Attack
, virtual public EC::Stats::Armor
{
public:
    static constexpr int  MAX_HP   = 90;
    static constexpr int  STRENGTH = 9;
    static constexpr Type TYPE     = "priest";

private:
    Level*            m_map       = nullptr;
    Dir               m_direction = Dir::DOWN;
    modlib::Inventory m_inventory;

public:
    Priest (
        Level *map,
        Tile *tile,
        modlib::BmClient *client
    )
    : Entity(TYPE, tile)
    , Health(MAX_HP, MAX_HP)
    , Attack(STRENGTH)
    , EC::Stats::Armor(0, 1)
    , m_map(map)
    {
        (void)client;

        m_inventory.addItem (
            modlib::ItemDef (
                "holyhammer",
                {
                    modlib::AbilityDef("crash"),
                    modlib::AbilityDef("divine_smite")
                }
            )
        );
        m_inventory.addItem (
            modlib::ItemDef (
                "holy_shield",
                {
                    modlib::AbilityDef("pray"),
                    modlib::AbilityDef("shieldsup")
                }
            )
        );
    }

    void
    rotate (
        Dir dir
    )
    {
        m_direction = dir;
    }

    Dir
    dirrection () const
    {
        return m_direction;
    }

    Level* 
    map ()
    {
        return m_map;
    }

    modlib::Inventory&
    inventory()
    {
        return m_inventory;
    }

    const modlib::Inventory&
    inventory()
    const
    {
        return m_inventory;
    }
};
