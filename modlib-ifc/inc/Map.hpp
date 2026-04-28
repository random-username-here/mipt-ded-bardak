#pragma once


#include "Vec2.hpp"
#include "modlib_mod.hpp"
#include "binmsg.hpp"

#include <cstdint>
#include <memory>
#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>

// namespace modlib is forbidden here. DONT UNCOMMENT, ILL FIND U :)


class Tile;

class Entity
{
public:
    using ID = uint64_t;

    // deprecated soon
    using Type = bmsg::Char64;

    Entity (Type type, Tile* tile);
    Entity (Type type, Tile* tile);

    ~Entity ();


    ID    getID   () const;
    Type  getType () const;
    
    Tile*    getTile     () const;
    Vec2D<>  getPosition () const;
    void     setTile     (Tile*   tile);
    void     setPosition (Vec2D<> position);

private:
    Type  m_type;
    ID    m_ID;
    Tile* m_tile;
};

class Level;

class Tile
{
public:
    // deprecated soon
    using Type = bmsg::Char64;

     Tile (Level* level, Vec2D<> position, Type type);
    ~Tile ();

    Level*   getLevel () const;
    Vec2D<>  getPos   () const;
    Type     getType  () const;
    void     setType  (Type type);

    void    addEntity (Entity*     entity);
    void removeEntity (Entity::ID  id);

    const std::unordered_map<Entity::ID, Entity*>& getEntityList () const;
private:
    Level*  m_level;
    Vec2D<> m_position;
    Type    m_type;

    std::unordered_map<Entity::ID, Entity*> m_EntityList;
};

class Level : public Mod 
{
public:
    using ID = uint64_t;

    ID      getLevelID () const;
    Vec2D<> getSize    () const;
    
                                  Tile       & getTile      (Vec2D<> position) const;
          std::vector<std::vector<Tile>     >& getTileMap   ()                 const;
    const std::unordered_set<     Tile::Type>& getTileTypes ()                 const;
    
    Entity::ID                                     addEntity       (Entity* entity, Vec2D<>  position) const;
    Entity::ID                                     addEntity       (Entity* entity, Tile   & tile    ) const;
    void                                        removeEntity       (Entity::ID id,  Vec2D<>  position) const;
    void                                        removeEntity       (Entity::ID id                    ) const;
    void                                        removeEntity       (Entity::ID id,  Tile   & tile    ) const;
                             Entity*                getEntity      (Entity::ID id                    ) const;
    const std::vector       <Entity*             >& getEntityList  ()                                  const;
    const std::unordered_map<Entity::Type, size_t>& getEntityTypes ()                                  const;

    void loadLevel (std::string_view path2level);

private:
    ID      m_levelID;
    Vec2D<> m_size;

    std::vector<std::vector<Tile>> m_tileMap;
    std::unordered_set<Tile::Type> m_tileTypes;

    std::unordered_map<Entity::ID,   Entity*> m_entityList;
    std::unordered_map<Entity::Type, size_t > m_entityTypes;
};
