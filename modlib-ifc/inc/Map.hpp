#pragma once


#include "Vec2.hpp"
#include "Event.hpp"
#include "binmsg.hpp"

#include <cstdint>
#include <vector>
#include <unordered_set>

namespace modlib { namespace Map {
using   Coordinate = int;          // a special alias for representing coordinate
using   VecXY = Vec2D<Coordinate>; // a special alias for representing 2D coordinates
#define BADXY  modlib::Map::VecXY (INT32_MAX, INT32_MAX)

using   ID    = uintptr_t;


class Tile;
class Level;

class Entity
{
public:
                Entity (Tile* tile = nullptr);
    virtual ~Entity ();

    virtual uint8_t destroy ();

    Tile*  getTile () const;    
    void   setTile (Tile* tile);

    Event<Tile*, Tile*> EvEntityMoved;
    Event<>             EvBeingDeconstructed;

private:
    Tile* m_tile;
};

class Tile
{
public:
    using Type = bmsg::Char64;

                Tile (Level* level, VecXY position, Type type);
    virtual ~Tile ();

    Level* getLevel () const;
    VecXY  getPos   () const;
    Type   getType  () const;

    void   setLevel (Level* level, VecXY position=BADXY);
    void   setPos   (VecXY  position);
    void   setType  (Type type);

    void    addEntity (Entity* entity);
    void removeEntity (Entity* entity);

    std::unordered_set<Entity*>& getEntityList ();

    void finalCleaning ();

    Event<Level*, VecXY, Level*, VecXY> EvLvlChanged;
    Event<VecXY,  VecXY>                EvPosChanged;
    Event<Entity*>                      EvEntityHasCome;
    Event<Entity*>                      EvEntityHasGone;

    Event<Type, Type>                   EvTypeChanged;

    Event<>                             EvCleaning;
    Event<>                             EvBeingDeconstructed;
private:
    Type         m_type;
    Level*       m_level;
    VecXY        m_position;

    std::unordered_set<Entity*> m_EntityList;
};

class Level
{
public:
    virtual ~Level () {};
    
    void finalCleaning ();

    VecXY getSize () const;

    Tile*                           getTile    (VecXY position);
    std::vector<std::vector<Tile>>& getTileMap ();

    void loadLevel (std::string_view path2level);

    Event<> EvCleaning;
    Event<> EvLevelLoaded;

protected:
    std::vector<std::vector<Tile>> m_tileMap;
};

} } // namespace modlib::Map
