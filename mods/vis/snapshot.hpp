#pragma once

#include "Map.hpp"
#include "person_base.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>
#include <iostream>

namespace vis {

struct UnitSnap {
    modlib::Entity *person;

    int x;
    int y;
    int hp;
    int maxHp;
    size_t id;

    UnitSnap()
        : x(0)
        , y(0)
        , hp(0)
        , maxHp(0)
        , id(0)
    {}
};

struct WorldSnap {
    int w;
    int h;
    uint64_t tick;
    bool valid;

    std::vector<bool> walkable;
    std::vector<UnitSnap> entities;

    WorldSnap()
        : w(0)
        , h(0)
        , tick(0)
        , valid(false)
    {}
};

class Snapshotter {
public:
    WorldSnap capture(modlib::Level *map) const {
        WorldSnap snap;

        if (!map) return snap;

        modlib::Vec2i sz = map->getSize();

        snap.w = sz.x;
        snap.h = sz.y;
        snap.valid = (sz.x > 0 && sz.y > 0);

        if (!snap.valid) return snap;

        snap.walkable.resize(static_cast<size_t>(snap.w * snap.h), false);

        for (int y = 0; y < snap.h; ++y) {
            for (int x = 0; x < snap.w; ++x) {
                modlib::Tile *tile = map->getTile(modlib::Vec2i{x, y});
                const size_t idx = static_cast<size_t>(y * snap.w + x);

                if (!tile) {
                    snap.walkable[idx] = false;
                    continue;
                }

                snap.walkable[idx] = tile->getType() != modlib::Tile::BasicTypes::WALL;

                const auto &entities = tile->getEntityList();

                for (auto &[id, entity] : entities) {
                    if (!entity) continue;

                    UnitSnap us;
                    us.x       = x;
                    us.y       = y;

                    int hp = 100;
                    int maxHp = 100;
                    auto *health = dynamic_cast<EC::Stats::Health *>(entity);
                    if (health) {
                        hp = static_cast<int>(health->getCurrentHP());
                        maxHp = static_cast<int>(health->getMaxHP());
                    }
                    us.hp      = hp;
                    us.maxHp   = maxHp;
                    us.id      = entity->getID();

                    us.person = dynamic_cast<modlib::Entity *>(entity);

                    snap.entities.push_back(us);
                }
            }
        }

        return snap;
    }
};

} // namespace vis
