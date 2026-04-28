#include "Map.hpp"
#include <random>
#include <stdexcept>

Entity::Entity (Entity::Type type, Tile* tile) : m_type (type)
{
    m_ID = rand ();

    if (tile)
    {
        tile->addEntity (this);
    }
    m_tile = tile;
}

Entity::~Entity ()
{
    if (m_tile) m_tile->getLevel ()->removeEntity (m_ID);
}


Entity::ID Entity::getID () const
{
    return m_ID;
}

Entity::Type Entity::getType () const
{
    return m_type;
}

Tile* Entity::getTile () const
{
    return m_tile;
}

Vec2D<> Entity::getPosition () const
{
    return m_tile->getPos ();
}


void Entity::setTyleType (Tile* tile)
{
    m_tile->removeEntity (m_ID);
    m_tile = tile;
    if (m_tile)
    {
        m_tile->addEntity (this);
    }
}

void Entity::setPosition (Vec2D<> position)
{
    if (m_tile)
    {
        m_tile->removeEntity (m_ID);
        m_tile = &(m_tile->getLevel ()->getTile (position));
        m_tile->addEntity (this);
    }
    else
    {
        throw std::runtime_error ("Entity must be added to the Level");
    }
}