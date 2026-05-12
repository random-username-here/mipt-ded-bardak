#include "Map.hpp"
#include <cstdint>

namespace modlib { namespace Map
{


Entity::Entity (
    Tile* tile
)
{
    if (tile)
    {
        tile->addEntity (this);
    }
    m_tile = tile;
}

Entity::~Entity ()
{
    if (m_tile)
    {
        m_tile->removeEntity (this);
    }

    EvBeingDeconstructed.emit ();
}

Tile* 
Entity::getTile () const
{
    return m_tile;
}

void 
Entity::setTile (
    Tile* newTile
)
{
    if (m_tile)
    {
        m_tile->removeEntity (this);
    }

    Tile* oldTile = m_tile;
    m_tile = newTile;

    if (m_tile)
    {
        m_tile->addEntity (this);
    }

    EvEntityMoved.emit (
        oldTile,
        newTile
    );
}

uint8_t
Entity::destroy ()
{
    return 0;
}

} }
