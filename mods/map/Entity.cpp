#include "Map.hpp"
#include <random>

Entity::Entity (Entity::Type type, Tile* tile, Tile::Layer layer) : m_type (type)
{
    m_ID = rand ();

    tile->addEntity (this, layer);
    m_tile = tile;
}
Entity::Entity (Entity::Type type, Tile* tile, uint8_t layer) : m_type (type)
{
    m_ID = rand ();

    tile->addEntity (this, layer);
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
    return m_tile->getPos ()
}


void Entity::setTile (Tile* tile, uint8_t layer)
{
    m_tile->removeEntity (m_ID);
    m_tile = tile;
    m_tile->addEntity    (this, layer);
}

void Entity::setTile (Tile* tile, Tile::Layer layer)
{
    m_tile->removeEntity (m_ID);
    m_tile = tile;
    m_tile->addEntity    (this, layer);
}

void Entity::setPosition (Vec2D<> position, uint8_t layer)
{
    m_tile->removeEntity (m_ID);
    m_tile = &(m_tile->getLevel ()->getTile (position));
    m_tile->addEntity (this, layer);
}

void Entity::setPosition (Vec2D<> position, Tile::Layer layer)
{
    m_tile->removeEntity (m_ID);
    m_tile = &(m_tile->getLevel ()->getTile (position));
    m_tile->addEntity (this, layer);
}