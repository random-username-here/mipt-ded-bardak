#include "Map.hpp"


Level::ID Level::getLevelID () const
{
    return m_levelID;
}

Vec2D<> Level::getSize () const
{
    return Vec2D<> (m_tileMap.size (), m_tileMap[0].size ());
}


Tile& Level::getTile (Vec2D<> position)
{
    return m_tileMap[position.m_x][position.m_y]; 
}

const std::vector<std::vector<Tile>>& Level::getTileMap ()
{
    return m_tileMap;
}

const std::unordered_map<Tile::Type, size_t>& Level::getTileTypes () const
{
    return m_tileTypes;
}


Entity::ID Level::newEntity (Entity* entity, Vec2D<> position)
{
    m_entityList [entity->getID   ()] = entity;
    m_entityTypes[entity->getType ()]++;

    m_tileMap[position.m_x][position.m_y].addEntity (entity);
}

Entity::ID Level::newEntity (Entity* entity, Tile& tile)
{
    m_entityList [entity->getID   ()] = entity;
    m_entityTypes[entity->getType ()]++;

    tile.addEntity (entity);
}

void Level::removeEntity (Entity::ID id)
{
    if (m_entityList.contains (id) == false)
    {
        return;
    }

    m_entityList[id]->getTile ()->removeEntity (id);
    
    m_entityTypes[m_entityList[id]->getType ()]--;
    m_entityList.erase (id);
}


Entity* Level::getEntity (Entity::ID id)
{
    return m_entityList[id];
}

const std::unordered_map<Entity::ID, Entity*>& Level::getEntityList ()
{
    return m_entityList;
}

const std::unordered_map<Entity::Type, size_t>& Level::getEntityTypes () const
{
    return m_entityTypes;
}