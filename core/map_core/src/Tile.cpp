#include <cassert>
#include "Map.hpp"

namespace modlib { namespace Map
{

Tile::Tile (
    Level* level,
    VecXY position,
    Type type
)
: m_type (type)
, m_level (level)
, m_position (position)
{

}

Tile::~Tile ()
{
    EvBeingDeconstructed.emit ();
}

Level*
Tile::getLevel () const
{
    return m_level;
}

VecXY
Tile::getPos () const
{
    return m_position;
}

std::unordered_set<Entity*>& 
Tile::getEntityList ()
{
    return m_EntityList;
}


void
Tile::removeEntity (
    Entity* entity
)
{
    if (m_EntityList.find(entity) != m_EntityList.end())
    {
        m_EntityList.erase (entity);
        entity->setTile (nullptr);

        EvEntityHasGone.emit (entity);
    }
}

void
Tile::addEntity (
    Entity* entity
)
{
    assert (entity);

    m_EntityList.insert  (entity);
    EvEntityHasCome.emit (entity);
}

void
Tile::setLevel (
    Level* level,
    VecXY position
)
{
    Level* oldLvl = m_level;
    m_level = level;

    VecXY oldPos = m_position;
    if (level)
    {
        if (level && position != BADXY)
        {
            m_position = position;
        }
    }
    else
    {
        m_position = BADXY;
    }

    EvLvlChanged.emit (
        oldLvl,
        oldPos,
        m_level,
        m_position
    );
}

void
Tile::setPos (
    VecXY position
)
{
    VecXY oldPos = m_position;
    m_position = position;

    EvPosChanged.emit (
        oldPos, 
        m_position
    );
}

Tile::Type
Tile::getType () const
{
    return m_type;
}

void
Tile::setType (
    Type type
)
{
    Type oldType = m_type;
    m_type = type;

    EvTypeChanged.emit (oldType, type);
}

void
Tile::finalCleaning ()
{
    EvCleaning.emit ();

    for (Entity* entity : m_EntityList)
    {
        delete entity;
    }
}

} }