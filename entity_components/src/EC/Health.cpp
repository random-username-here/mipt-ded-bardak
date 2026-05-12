#include "EC.hpp"

using namespace EC;

Stats::Health::Health (
    HP currentHP,
    HP maxHP
)
: m_currentHP (currentHP)
, m_maxHP (maxHP)
{}

Stats::Health::~Health ()
{
}

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
        Common::Damage::DMG dmg = m_currentHP - maxHP;
        inflictDmg (
            Common::Damage (
                Common::Damage::PURE,
                dmg
            )
        );
    }

    HP oldMax = m_maxHP;
    m_maxHP = maxHP;
    EvMaxHPChanged.emit (oldMax, m_maxHP);
}

Stats::Health::HP
Stats::Health::inflictDmg (
    Common::Damage damage
)
{
    if (damage.m_type != Common::Damage::PURE)
    {
        if (Stats::Armor* armor = dynamic_cast<Stats::Armor*> (this))
        {
            damage.m_damage = armor->calculateDamage (damage);
        }
    }
    
    if (damage.m_damage > m_currentHP)
    {
        m_currentHP = 0;
        
        EvDamaged.emit (damage.m_damage);
        EvDeath.emit ();

        return 0;
    }
    else
    {
        m_currentHP -= damage.m_damage;
        EvDamaged.emit (damage.m_damage);

        return m_currentHP;
    }
}

Stats::Health::HP 
Stats::Health::heal (
    Stats::Health::HP healed
)
{
    Stats::Health::HP clamped = std::clamp<Stats::Health::HP> (
        healed,
        0,
        m_maxHP - m_currentHP
    );
    
    m_currentHP += clamped;
    EvHealed.emit (clamped);

    return m_currentHP;
}