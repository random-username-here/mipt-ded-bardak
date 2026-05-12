#include "EC.hpp"
#include "Map.hpp"
using namespace EC::Actions;


Movable::Movable (
    modlib::Map::Tile* tile,
    modlib::Map::Coordinate speed
)
: modlib::Map::Entity (tile)
, m_TPT (speed)
{}

modlib::Map::Coordinate
Movable::getSpeed () const
{
    return m_TPT;
}

void
Movable::setSpeed (
    modlib::Map::Coordinate speed
)
{
    modlib::Map::Coordinate oldSpeed = m_TPT;
    m_TPT = speed;
    
    EvSpeedChanged.emit(oldSpeed, m_TPT);
}