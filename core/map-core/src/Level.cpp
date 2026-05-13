#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

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

bool Level::isWalkable(Vec2i position) const
{
    Vec2i size = getSize();

    if (position.x < 0 || position.x >= size.x ||
        position.y < 0 || position.y >= size.y)
    {
        return false;
    }

    const Tile& tile = m_tileMap[position.x][position.y];
    if (tile.getType() == Tile::BasicTypes::WALL)
    {
        return false;
    }

    for (const auto& [id, entity] : tile.getEntityList())
    {
        (void) id;
        if (entity && entity->getType() == Entity::BasicTypes::ROOT)
        {
            return false;
        }
    }

    return true;
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

void Level::loadLevel(std::string_view path2level)
{
    std::string path(path2level.data(), path2level.size());

    std::ifstream file(path.c_str());

    if (!file) {
        throw std::runtime_error("Map loader: failed to open level file: " + path);
    }

    std::string mapName;
    std::getline(file, mapName);

    size_t width = 0;
    size_t height = 0;

    if (!(file >> width >> height)) {
        throw std::runtime_error("Map loader: expected '<width> <height>'");
    }

    if (width == 0 || height == 0) {
        throw std::runtime_error("Map loader: map size must be non-zero");
    }

    std::string word;

    if (!(file >> word) || word != "DECLARE") {
        throw std::runtime_error("Map loader: expected DECLARE section");
    }

    std::unordered_map<size_t, Tile::Type> declaredTypes;

    while (file >> word) {
        if (word == "END") {
            break;
        }

        size_t code = 0;

        try {
            code = std::stoull(word);
        }
        catch (const std::exception &) {
            throw std::runtime_error("Map loader: invalid tile code in DECLARE section");
        }

        std::string typeName;

        if (!(file >> typeName)) {
            throw std::runtime_error("Map loader: invalid DECLARE entry");
        }

        declaredTypes.emplace(code, Tile::Type(typeName.c_str()));
    }

    if (declaredTypes.empty()) {
        throw std::runtime_error("Map loader: DECLARE section is empty");
    }

    std::vector<std::vector<size_t>> matrix(height, std::vector<size_t>(width));

    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            size_t code = 0;

            if (!(file >> code)) {
                throw std::runtime_error("Map loader: not enough tile ids in matrix");
            }

            if (declaredTypes.find(code) == declaredTypes.end()) {
                throw std::runtime_error("Map loader: matrix uses undeclared tile id");
            }

            matrix[y][x] = code;
        }
    }

    std::vector<Entity::ID> entitiesToRemove;
    entitiesToRemove.reserve(m_entityList.size());

    for (const auto &[id, entity] : m_entityList) {
        (void)entity;
        entitiesToRemove.push_back(id);
    }

    for (Entity::ID id : entitiesToRemove) {
        removeEntity(id);
    }

    m_tileMap.clear();
    m_tileTypes.clear();

    m_tileMap.reserve(width);

    for (size_t x = 0; x < width; ++x) {
        std::vector<Tile> column;
        column.reserve(height);

        for (size_t y = 0; y < height; ++y) {
            Tile::Type type = declaredTypes.at(matrix[y][x]);

            column.emplace_back(*this, Vec2i(static_cast<int>(x), static_cast<int>(y)), type);

            ++m_tileTypes[type];

            if (m_tileTypes[type] == 1) {
                EvTileTypeNew.emit(type);
            }
        }

        m_tileMap.push_back(std::move(column));
    }

    ++m_levelID;
    EvLevelLoaded.emit();
}


}; // namepsace modlib
