#pragma once

#include "Map.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace vis {

// WARN: inferred from game logic and hardcoded = bad
static const int PERSON_MAX_HP = 100;

struct UnitSnap {
    int x;
    int y;
    int hp;
    int maxHp;
    size_t id;

    UnitSnap()
        : x(0)
        , y(0)
        , hp(0)
        , maxHp(PERSON_MAX_HP)
        , id(0)
    {}
};

struct WorldSnap {
    int w;
    int h;
    uint64_t tick;
    bool valid;

    std::vector<bool> walkable;
    std::vector<UnitSnap> units;

    WorldSnap()
        : w(0)
        , h(0)
        , tick(0)
        , valid(false)
    {}
};

class Snapshotter {
public:
    WorldSnap capture(modlib::Map *map) const {
        WorldSnap snap;

        if (!map) return snap;

        modlib::Vec2i sz = map->size();

        snap.w = sz.x;
        snap.h = sz.y;
        snap.valid = (sz.x > 0 && sz.y > 0);

        if (!snap.valid) return snap;

        snap.walkable.resize(static_cast<size_t>(snap.w * snap.h), false);

        for (int y = 0; y < snap.h; ++y) {
            for (int x = 0; x < snap.w; ++x) {
                modlib::Tile *tile = map->at(modlib::Vec2i{x, y});
                const size_t idx = static_cast<size_t>(y * snap.w + x);

                if (!tile) {
                    snap.walkable[idx] = false;
                    continue;
                }

                snap.walkable[idx] = tile->type() != modlib::Tile::BasicType::Wall;

                const std::vector<modlib::Unit *> &units = tile->units();

                for (size_t i = 0; i < units.size(); ++i) {
                    modlib::Unit *u = units[i];
                    if (!u) continue;

                    UnitSnap us;
                    us.x     = x;
                    us.y     = y;
                    us.hp    = u->hp();
                    us.maxHp = PERSON_MAX_HP;
                    us.id    = u->id();

                    snap.units.push_back(us);
                }
            }
        }

        return snap;
    }
};

} // namespace vis
