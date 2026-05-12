#include "Map.hpp"
#include <cstddef>
#include <iostream>

namespace modlib { namespace Map {


VecXY 
Level::getSize () const 
{
    if (m_tileMap.size() == 0) return {0, 0};

    return VecXY (m_tileMap.size (), m_tileMap[0].size ());
}


Tile* 
Level::getTile (
    VecXY position
)
{
    VecXY size = getSize();

    if (
        position.x < 0 || position.x >= size.x || 
        position.y < 0 || position.y >= size.y
    )
    {
        return nullptr;
    }

    return &m_tileMap[position.x][position.y];
}

std::vector<std::vector<Tile>>&
Level::getTileMap ()
{
    return m_tileMap;
}

void
Level::loadLevel (
    std::string_view path2level
)
{
    std::cerr << "`loadLevel` is not implemented yet\n";
}

void
Level::finalCleaning ()
{
    EvCleaning.emit ();

    VecXY size = getSize();

    for (Coordinate x = 0; x < size.x; x++)
    {
        for (Coordinate y = 0; y < size.y; y++)
        {
            m_tileMap[x][y].finalCleaning ();
        }
    }
}


} }