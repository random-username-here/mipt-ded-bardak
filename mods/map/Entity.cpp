#include "Map.hpp"
#include <random>

Entity::Entity (Entity::Type type, Tile* tile) : m_type (type)
{
    m_ID = rand ();

    tile->addEntity (this);
    m_tile = tile;
}

Entity::~Entity ()
{
    m_tile->getLevel ()->removeEntity (m_ID);
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


void Entity::setTile (Tile* tile)
{
    m_tile->removeEntity (m_ID);
    m_tile = tile;
    m_tile->addEntity    (this);
}

void Entity::setPosition (Vec2D<> position)
{
    m_tile->removeEntity (m_ID);
    m_tile = &(m_tile->getLevel ()->getTile (position));
    m_tile->addEntity (this);
}