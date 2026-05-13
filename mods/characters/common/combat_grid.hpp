#pragma once

#include "Map.hpp"

#include <string_view>
#include <vector>

namespace combat_grid {

inline int iabs(int v)
{
    return v < 0 ? -v : v;
}

inline bool visibleOffset(int dx, int dy)
{
    /*
     * Literal 7x7 mask:
     *
     * 0 0 1 1 1 0 0
     * 0 1 1 1 1 1 0
     * 1 1 1 1 1 1 1
     * 1 1 1 1 1 1 1
     * 1 1 1 1 1 1 1
     * 0 1 1 1 1 1 0
     * 0 0 1 1 1 0 0
     */
    if (dx < -3 || dx > 3 || dy < -3 || dy > 3) {
        return false;
    }

    return dx * dx + dy * dy <= 10;
}

inline bool visible(modlib::Vec2i from, modlib::Vec2i to)
{
    return visibleOffset(to.x - from.x, to.y - from.y);
}

inline bool isRoot(const modlib::Entity *entity)
{
    return entity && entity->getType() == modlib::Entity::BasicTypes::ROOT;
}

inline bool isCombatant(const modlib::Entity *entity)
{
    return entity && !isRoot(entity) && dynamic_cast<const EC::Stats::Health *>(entity) != nullptr;
}

inline bool blocksMovement(const modlib::Entity *entity)
{
    return isRoot(entity) || isCombatant(entity);
}

inline bool canEnter(modlib::Level *map, modlib::Vec2i position, const modlib::Entity *self = nullptr)
{
    if (map == nullptr) {
        return false;
    }

    modlib::Tile *tile = map->getTile(position);
    if (tile == nullptr || tile->getType() == modlib::Tile::BasicTypes::WALL) {
        return false;
    }

    for (const auto &[id, entity] : tile->getEntityList()) {
        (void)id;

        if (entity == nullptr || entity == self) {
            continue;
        }

        if (blocksMovement(entity)) {
            return false;
        }
    }

    return true;
}

inline bool inMooreRange(modlib::Vec2i from, modlib::Vec2i to)
{
    const int dx = iabs(from.x - to.x);
    const int dy = iabs(from.y - to.y);

    return dx <= 1 && dy <= 1 && (dx + dy) > 0;
}

inline bool inVonNeumannRange(modlib::Vec2i from, modlib::Vec2i to)
{
    return iabs(from.x - to.x) + iabs(from.y - to.y) == 1;
}

inline bool inArcherRange(modlib::Vec2i from, modlib::Vec2i to)
{
    const int dx = iabs(from.x - to.x);
    const int dy = iabs(from.y - to.y);

    if (dx == 0 && dy == 0) {
        return false;
    }

    /*
     * Literal 5x5 mask:
     *
     * 0 1 1 1 0
     * 1 1 1 1 1
     * 1 1 1 1 1
     * 1 1 1 1 1
     * 0 1 1 1 0
     */
    return dx <= 2 && dy <= 2 && !(dx == 2 && dy == 2);
}

inline std::vector<modlib::Vec2i> visibleOffsets()
{
    std::vector<modlib::Vec2i> out;

    for (int dx = -3; dx <= 3; ++dx) {
        for (int dy = -3; dy <= 3; ++dy) {
            if (visibleOffset(dx, dy)) {
                out.push_back({dx, dy});
            }
        }
    }

    return out;
}

inline bmsg::Char64 entityTypeName(const modlib::Entity *entity)
{
    if (entity == nullptr) {
        return bmsg::Char64();
    }

    return entity->getType();
}

} // namespace combat_grid
