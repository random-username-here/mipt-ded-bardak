#pragma once


#include "Vec2.hpp"
#include "modlib_mod.hpp"
#include "binmsg.hpp"
#include "Event.hpp"

#include "ECbasis.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace modlib {

class Tile;

class Entity : public EC::Entity
{
public:
    Entity (Type type, Tile* tile);

    virtual ~Entity ();

    Tile*    getTile     () const;                                                                                  // Please refrain from using this method for safety reasons
    Vec2i    getPosition () const;
    void     setTile     (Tile*   tile);
    void     setPosition (Vec2i position);

    Event<Vec2i> EvEntityMoved;

private:
    Tile* m_tile;
};

class Level;

class Tile
{
public:
    using Type = bmsg::Char64;

    struct BasicTypes {
       static const inline Type WALL = Type("wall");
       static const inline Type EMPTY = Type("empty");
    };

     Tile (Level& level, Vec2i position, Type type);
    ~Tile ();

    Level&   getLevel () const;
    Vec2i  getPos   () const;
    Type     getType  () const;
    void     setType  (Type type);

    void    addEntity (Entity*     entity);
    void removeEntity (Entity::ID  id);

    const std::unordered_map<Entity::ID, Entity*>& getEntityList () const;                                      // Please refrain from using this method for safety reasons

    Event<Type>       EvTileTypeChanged;
    Event<Entity::ID> EvEntityHasCome;
    Event<Entity::ID> EvEntityHasGone;
private:
    Level&  m_level;
    Vec2i m_position;
    Type    m_type;

    std::unordered_map<Entity::ID, Entity*> m_EntityList;
};

class Level : public Mod
{
    friend void Tile::setType (Tile::Type type);
public:
    using ID = uint64_t;

    virtual ~Level() {};

    ID getLevelID() const;
    Vec2i getSize() const;

    Tile *getTile(Vec2i position);
    const std::vector       <std::vector<Tile>             >& getTileMap   ();
    const std::unordered_map<Tile::Type, size_t, bmsg::Char64Hasher>& getTileTypes ()                 const;

    Entity::ID                                      newEntity       (Entity* entity, Vec2i  position);
    Entity::ID                                      newEntity       (Entity* entity, Tile   *tile    );
    void                                         removeEntity       (Entity::ID id                    );
                             Entity*                 getEntity      (Entity::ID id                    );        // Please refrain from using this method for safety reasons
    const std::unordered_map<Entity::ID,   Entity*>& getEntityList  ();                                         // Please refrain from using this method for safety reasons
    const std::unordered_map<Entity::Type, size_t, bmsg::Char64Hasher>& getEntityTypes ()                                  const;

    void loadLevel (std::string_view path2level);


    Event<>             EvLevelLoaded;

    Event<Tile::Type>   EvTileTypeNew;
    Event<Tile::Type>   EvTileTypeExpired;

    Event<Entity::ID>   EvEntitySpawned;
    Event<Entity*>      EvEntityDespawned;

    Event<Entity::Type> EvEntityTypeNew;
    Event<Entity::Type> EvEntityTypeExpired;
protected:
    ID m_levelID;

    std::vector<std::vector<Tile>             > m_tileMap;
    std::unordered_map<Tile::Type, size_t, bmsg::Char64Hasher> m_tileTypes;

    std::unordered_map<Entity::ID,   Entity*> m_entityList;
    std::unordered_map<Entity::Type, size_t, bmsg::Char64Hasher> m_entityTypes;
};

} // namespace modlib
