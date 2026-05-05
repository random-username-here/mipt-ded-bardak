#include <iostream>

#include "Map.hpp"

namespace modlib {


Level::ID Level::getLevelID () const 
{ 
    return m_levelID; 
}

Vec2i Level::getSize () const 
{
    if (m_tileMap.size() == 0) return {0, 0};

    return Vec2i (m_tileMap.size (), m_tileMap[0].size ());
}


Tile *Level::getTile(Vec2i position)
{
    Vec2i size = getSize();

    if (position.x < 0 || position.x >= size.x || 
        position.y < 0 || position.y >= size.y) 
    {
        return nullptr;
    }

    return &m_tileMap[position.x][position.y];
}

const std::vector<std::vector<Tile>>& Level::getTileMap ()
{
    return m_tileMap;
}

const std::unordered_map<Tile::Type, size_t, bmsg::Char64Hasher>& Level::getTileTypes () const
{
    return m_tileTypes;
}


Entity::ID Level::newEntity (Entity* entity, Vec2i position)
{
    m_entityList [entity->getID   ()] = entity;

    m_entityTypes[entity->getType ()]++;
    if (m_entityTypes[entity->getType ()] == 1)
    {
        EvEntityTypeNew.emit (entity->getType ());
    }

    m_tileMap[position.x][position.y].addEntity (entity);
    EvEntitySpawned.emit (entity->getID ());

    return entity->getID();
}

Entity::ID Level::newEntity (Entity* entity, Tile* tile)
{
    assert(tile);

    m_entityList [entity->getID   ()] = entity;
    
    m_entityTypes[entity->getType ()]++;
    if (m_entityTypes[entity->getType ()] == 1)
    {
        EvEntityTypeNew.emit (entity->getType ());
    }

    tile->addEntity (entity);
    EvEntitySpawned.emit (entity->getID ());

    return entity->getID();
}

void Level::removeEntity (Entity::ID id)
{
    if (m_entityList.find(id) == m_entityList.end())
    {
        return;
    }

    Entity* entity = m_entityList[id];

    entity->getTile ()->removeEntity (id);
    
        m_entityTypes[entity->getType ()]--;
    if (m_entityTypes[entity->getType ()] == 0)
    {
        EvEntityTypeExpired.emit (entity->getType ());
    }

    m_entityList.erase (id);
    EvEntityDespawned.emit (entity);
}


Entity* Level::getEntity (Entity::ID id)
{
    return m_entityList[id];
}

const std::unordered_map<Entity::ID, Entity*>& Level::getEntityList ()
{
    return m_entityList;
}

const std::unordered_map<Entity::Type, size_t, bmsg::Char64Hasher>& Level::getEntityTypes () const
{
    return m_entityTypes;
}

void Level::loadLevel (std::string_view path2level) {
    std::cerr << id() << " `loadLevel` is not implemented yet\n";
}


}; // namepsace modlib