#pragma once

#include "BmServerModule.hpp"
#include "Map.hpp"

#include <cstdlib>

using namespace modlib;

enum class KnightDir {
    down  = 0,
    up    = 1,
    left  = 2,
    right = 3,
};

inline KnightDir knightDirFromDelta(Vec2i delta)
{
    if (std::abs(delta.x) > std::abs(delta.y)) {
        return delta.x < 0 ? KnightDir::left : KnightDir::right;
    }
    if (delta.y != 0) {
        return delta.y < 0 ? KnightDir::up : KnightDir::down;
    }
    return KnightDir::down;
}

class Knight :
    virtual public modlib::Entity,
    virtual public EC::Stats::Health,
    virtual public EC::Stats::Attack
{
public:
    static constexpr int MAX_HP     = 80;
    static constexpr int CURRENT_HP = 80;
    static constexpr int STRENGTH   = 10;
    static constexpr Type KNIGHT_TYPE = "knight";

private:
    Level    *map_ = nullptr;
    KnightDir dir_ = KnightDir::down;

public:
    Knight(Level *map, Tile *tile, modlib::BmClient *client)
        : Entity(KNIGHT_TYPE, tile)
        , Health(CURRENT_HP, MAX_HP)
        , Attack(STRENGTH)
        , map_(map)
    {
        (void)client;
    }

    void rotate(KnightDir dir)
    {
        dir_ = dir;
    }

    KnightDir dir() const
    {
        return dir_;
    }

    Level *map()
    {
        return map_;
    }
};
