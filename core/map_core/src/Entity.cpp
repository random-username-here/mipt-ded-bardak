#include <random>
#include <stdexcept>
#include "Map.hpp"

namespace modlib::Map
{


Entity::Entity (Tile* tile)
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

Tile* Entity::getTile () const
{
    return m_tile;
}

void Entity::setTile (Tile* newTile)
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

}
