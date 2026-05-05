#include "Map.hpp"
#include <algorithm>


Tile::Tile (Level& level, Vec2D<> position, Type type) : m_level (level), m_position (position), m_type (type) 
{

}

Tile::~Tile ()
{

}

Level& Tile::getLevel () const
{
    return m_level;
}

Vec2D<> Tile::getPos () const
{
    return m_position;
}

Tile::Type Tile::getType () const
{
    return m_type;
}

void Tile::setType (Type type)
{
    m_level.m_tileTypes[m_type]--;
    if (m_level.m_tileTypes[m_type] == 0)
    {
        m_level.EvTileTypeExpired.emit (m_type);
    }

    m_type = type;
    
    if (m_level.m_tileTypes[m_type] == 0)
    {
        m_level.EvTileTypeNew.emit (m_type);
    }
    m_level.m_tileTypes[m_type]++;

    EvTileTypeChanged.emit (type);
}

const std::unordered_map<Entity::ID, Entity*>& Tile::getEntityList () const
{
    return m_EntityList;
}


void Tile::removeEntity (Entity::ID id)
{
    if (m_EntityList.contains (id))
    {
        m_EntityList[id]->setTile (nullptr);
        m_EntityList.erase (id);

        EvEntityHasGone.emit (id);
    }
}

void Tile::addEntity (Entity* entity)
{
    m_EntityList[entity->getID ()] = entity;

    EvEntityHasCome.emit (entity->getID ());
}