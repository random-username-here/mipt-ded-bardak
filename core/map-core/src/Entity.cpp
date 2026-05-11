#include <random>
#include <stdexcept>
#include "Map.hpp"

namespace modlib {


Entity::Entity (EC::Entity::Type type, Tile* tile) : EC::Entity(type)
{
    if (tile)
    {
        tile->addEntity (this);
    }
    m_tile = tile;
}

Entity::~Entity ()
{
    if (m_tile) m_tile->getLevel ().removeEntity (getID ());
}

Tile* Entity::getTile () const
{
    return m_tile;
}

Vec2i Entity::getPosition () const
{
    return m_tile->getPos ();
}

void Entity::setTile (Tile* tile)
{
    Vec2i oldPosition = m_tile->getPos ();

    m_tile->removeEntity (getID ());

    m_tile = tile;
    if (m_tile)
    {
        m_tile->addEntity (this);

        EvEntityMoved.emit (m_tile->getPos () - oldPosition);
    }
}

void Entity::setPosition (Vec2i position)
{
    if (m_tile)
    {
        Vec2i oldPosition = m_tile->getPos ();
        Tile* oldTile = m_tile;

        oldTile->removeEntity (getID ());

        m_tile = oldTile->getLevel().getTile(position);
        if (!m_tile)
        {
            throw std::runtime_error ("Target tile is out of bounds");
        }
        m_tile->addEntity (this);

        EvEntityMoved.emit (m_tile->getPos () - oldPosition);
    }
    else
    {
        throw std::runtime_error ("Entity must be added to the Level");
    }
}

} // namespace modlib
