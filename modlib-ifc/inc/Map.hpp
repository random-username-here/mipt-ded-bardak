#pragma once


#include "Vec2.hpp"
#include "modlib_mod.hpp"
#include "binmsg.hpp"
#include "Event.hpp"

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>

// namespace modlib is forbidden here. DONT UNCOMMENT, ILL FIND U :)


class Tile;

class Entity
{
public:
    using ID = uint64_t;
    using Type = bmsg::Char64;

    Entity (Type type, Tile* tile);

    ~Entity ();


    ID    getID   () const;
    Type  getType () const;
    
    Tile*    getTile     () const;                                                                                  // Please refrain from using this method for safety reasons
    Vec2D<>  getPosition () const;
    void     setTile     (Tile*   tile);
    void     setPosition (Vec2D<> position);

    Event<Vec2D<>> EvEntityMoved;

private:
    Type  m_type;
    ID    m_ID;
    Tile* m_tile;
};

class Level;

class Tile
{
public:
    using Type = bmsg::Char64;

     Tile (Level& level, Vec2D<> position, Type type);
    ~Tile ();

    Level&   getLevel () const;
    Vec2D<>  getPos   () const;
    Type     getType  () const;
    void     setType  (Type type);

    void    addEntity (Entity*     entity);
    void removeEntity (Entity::ID  id);

    const std::unordered_map<Entity::ID, Entity*>& getEntityList () const;                                      // Please refrain from using this method for safety reasons

    Event<Type> EvTileTypeChanged;
    Event<Entity::ID> EvEntityHasCome;
    Event<Entity::ID> EvEntityHasGone;
private:
    Level&  m_level;
    Vec2D<> m_position;
    Type    m_type;

    std::unordered_map<Entity::ID, Entity*> m_EntityList;
};

class Level : public Mod 
{
    friend void Tile::setType (Tile::Type type);
public:
    using ID = uint64_t;

    ID      getLevelID () const;
    Vec2D<> getSize    () const;
    
                                         Tile               & getTile      (Vec2D<> position);
    const std::vector       <std::vector<Tile>             >& getTileMap   ();
    const std::unordered_map<            Tile::Type, size_t>& getTileTypes ()                 const;
    
    Entity::ID                                      newEntity       (Entity* entity, Vec2D<>  position);
    Entity::ID                                      newEntity       (Entity* entity, Tile   & tile    );
    void                                         removeEntity       (Entity::ID id                    );
                             Entity*                 getEntity      (Entity::ID id                    );        // Please refrain from using this method for safety reasons
    const std::unordered_map<Entity::ID,   Entity*>& getEntityList  ();                                         // Please refrain from using this method for safety reasons
    const std::unordered_map<Entity::Type, size_t >& getEntityTypes ()                                  const;

    void loadLevel (std::string_view path2level);


    Event<>             EvLevelLoaded;

    Event<Tile::Type>   EvTileTypeNew;
    Event<Tile::Type>   EvTileTypeExpired;

    Event<Entity::ID>   EvEntitySpawned;
    Event<Entity*>      EvEntityDespawned;

    Event<Entity::Type> EvEntityTypeNew;
    Event<Entity::Type> EvEntityTypeExpired;
private:
    ID      m_levelID;

    std::vector       <std::vector<Tile>             > m_tileMap;
    std::unordered_map<            Tile::Type, size_t> m_tileTypes;

    std::unordered_map<Entity::ID,   Entity*> m_entityList;
    std::unordered_map<Entity::Type, size_t > m_entityTypes;
};