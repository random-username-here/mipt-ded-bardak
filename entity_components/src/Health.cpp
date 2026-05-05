#include "ECbasis.hpp"

using namespace EC;

Stats::Health::Health (
    HP currentHP,
    HP maxHP
)
: m_currentHP (currentHP)
, m_maxHP (maxHP)
{}

Stats::Health::HP 
Stats::Health::getCurrentHP () const
{
    return m_currentHP;
}

Stats::Health::HP 
Stats::Health::getMaxHP () const
{
    return m_maxHP;
}

void
Stats::Health::setMaxHP (
    Stats::Health::HP maxHP
)
{
    if (m_currentHP > maxHP)
    {
        HP dmg = m_currentHP - maxHP;
        inflictDmg (dmg);
    }

    m_maxHP = maxHP;
}

Stats::Health::HP
Stats::Health::inflictDmg (
    Stats::Health::HP damage
)
{
    if (damage > m_currentHP)
    {
        m_currentHP = 0;
        
        EvDamaged.emit (damage);
        EvDeath.emit ();

        return 0;
    }
    else
    {
        m_currentHP -= damage;
        EvDamaged.emit (damage);

        return m_currentHP;
    }
}

Stats::Health::HP 
Stats::Health::heal (
    Stats::Health::HP healed
)
{
    Stats::Health::HP delta = m_maxHP - m_currentHP;
    Stats::Health::HP clamped = healed >= delta ? delta : healed;
    
    m_currentHP += clamped;
    EvHealed.emit (clamped);

    return m_currentHP;
}