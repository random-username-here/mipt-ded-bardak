#include "IC.hpp"
using namespace EC::Items;


Durability::Durability (
    size_t maxDurability,
    size_t currentDurability
)
: m_maxDurability (maxDurability)
, m_durability (currentDurability)
{}

Durability::~Durability ()
{
    setCurrentDurability (0);
}


size_t
Durability::getCurrentDurability () const
{
    return m_durability;
}

size_t
Durability::getMaxDurability () const
{
    return m_maxDurability;
}


void 
Durability::modifyDurability (
    int modifier
)
{
    setCurrentDurability (
        std::clamp<size_t> (
            m_durability + modifier,
            0,
            m_maxDurability
        )
    ); 
}

void 
Durability::setCurrentDurability (
    size_t durability
)
{
    size_t oldDurability = m_durability;
    m_durability = durability;
    EvDurabilityChanged.emit (
        oldDurability,
        m_durability
    );

    if (durability == 0)
    {
        EvBroken.emit ();
    }
}

void
Durability::setMaxDurability (
    size_t durability
)
{
    if (durability > m_durability)
    {
        setCurrentDurability (durability);
    }
    
    size_t oldMaxDurability = m_maxDurability;
    m_maxDurability = durability;

    EvMaxDurabilityChanged.emit (
        oldMaxDurability,
        m_maxDurability
    );
}