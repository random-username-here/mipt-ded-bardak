#include <algorithm>

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
    const bool wasAlive = m_currentHP > 0;
    m_currentHP = std::max((HP)0, m_currentHP - damage);
    EvDamaged.emit(damage);
    if (wasAlive && m_currentHP == 0) EvDeath.emit();
    return m_currentHP;
}

Stats::Health::HP 
Stats::Health::heal (
    Stats::Health::HP healed
)
{
    HP prevHP = m_currentHP; 
    m_currentHP = std::clamp(m_currentHP + healed, (HP)0, m_maxHP);
    EvHealed.emit (m_currentHP - prevHP);
    return m_currentHP;
}
