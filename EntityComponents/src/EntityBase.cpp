#include "random"
#include "../inc/ECbasis.hpp"
using namespace EC;


Entity::Entity ()
{
    m_id = rand ();
}

Entity::~Entity ()
{
    EvEntityDeconstructed.emit ();
}

Entity::ID
Entity::getID () const
{
    return m_id;
}

Entity::Type
Entity::getType () const
{
    return Entity::Type ("NONE");
}