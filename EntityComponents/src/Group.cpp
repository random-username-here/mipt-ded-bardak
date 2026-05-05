#include "ECbasis.hpp"
using namespace EC;


void Social::Group::setGroupID (Social::Group::GID gid)
{
    m_groupID = gid;

    EvGroupChanged.emit ();
}

void Social::Group::setGroupID (bmsg::Char64 group)
{
    m_groupID = group.as_u64;

    EvGroupChanged.emit ();
}


Social::Group::GID Social::Group::getGroupID () const
{
    return m_groupID;
}