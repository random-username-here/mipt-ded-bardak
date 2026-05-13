#pragma once

#include "BmServerModule.hpp"
#include "Map.hpp"
#include "Event.hpp"

#include <cstdlib>

using namespace modlib;

enum class TankDir : int8_t {
    down  = 0,
    up    = 1,
    left  = 2,
    right = 3,
};

inline TankDir tankDirFromClient(int8_t dir)
{
    // API: 0 - left, 1 - right, 2 - down, 3 - up
    switch (dir) {
    case 0:
        return TankDir::left;
    case 1:
        return TankDir::right;
    case 2:
        return TankDir::down;
    case 3:
        return TankDir::up;
    default:
        return TankDir::right;
    }
}

inline TankDir tankDirFromDelta(Vec2i delta)
{
    if (std::abs(delta.x) > std::abs(delta.y)) {
        return delta.x < 0 ? TankDir::left : TankDir::right;
    }
    if (delta.y != 0) {
        return delta.y < 0 ? TankDir::up : TankDir::down;
    }
    return TankDir::down;
}

inline Vec2i tankDirDelta(TankDir dir)
{
    switch (dir) {
    case TankDir::up:
        return {0, -1};
    case TankDir::down:
        return {0, 1};
    case TankDir::left:
        return {-1, 0};
    case TankDir::right:
        return {1, 0};
    }
    return {0, 1};
}

class Tank :
    virtual public modlib::Entity,
    virtual public EC::Stats::Health,
    virtual public EC::Stats::Attack
{
public:
    static constexpr int MAX_HP     = 80;
    static constexpr int CURRENT_HP = 80;
    static constexpr int STRENGTH   = 10;
    static constexpr Type TANK_TYPE = "tank";

private:
    Level *map_ = nullptr;
    TankDir dir_ = TankDir::right;

public:
    using Damage = EC::Stats::Attack::Damage;

    // Emitted when the tank changes facing direction.
    Event<TankDir> EvRotated;

    Tank(Level *map, Tile *tile, modlib::BmClient *client)
        : Entity(TANK_TYPE, tile)
        , Health(CURRENT_HP, MAX_HP)
        , Attack(STRENGTH)
        , map_(map)
    {
        (void)client;
    }

    void rotate(TankDir dir)
    {
        if (dir_ == dir) {
            return;
        }
        dir_ = dir;
        EvRotated.emit(dir_);
    }

    TankDir dir() const
    {
        return dir_;
    }

    Level *map()
    {
        return map_;
    }
};
