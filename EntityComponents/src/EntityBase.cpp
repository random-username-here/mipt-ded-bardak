#include "ECbasis.hpp"
#include "random"


using namespace EC;

EntityBase::EntityBase ()
{
    m_id = rand ();
}

EntityBase::~EntityBase ()
{
    EvEntityDeconstructed.emit ();
}

EntityBase::ID
EntityBase::getID () const
{
    return m_id;
}

EntityBase::Type
EntityBase::getType () const
{
    return EntityBase::Type ("NONE");
}