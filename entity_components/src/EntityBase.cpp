#include "random"
#include "ECbasis.hpp"
using namespace EC;

Entity::Entity (Type type): m_id (rand ()), m_type(type) {};

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
    return m_type;
}
