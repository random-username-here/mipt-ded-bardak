#include "EC.hpp"

using namespace EC;

Stats::Armor::Armor (AP armor)
    : m_armor (armor), m_resistance (calculateResist (armor)) {}

float
Stats::Armor::calculateResist (AP armor) const
{
    float armorCoeff = .06 * armor;
    return (1 - armorCoeff) / (1 + std::abs (armorCoeff));
}

Common::Damage::DMG
Stats::Armor::calculateDamage (
    const Common::Damage& damage
) const
{
    return damage.m_damage * m_resistance;
}

float
Stats::Armor::setArmor (AP armor)
{
    AP oldArmor = m_armor;
    m_armor      = armor;

    float oldResistance = m_resistance;
    m_resistance = calculateResist (m_armor);

    EvArmorChanged.emit (oldArmor, oldResistance, m_armor, m_resistance);

    return m_resistance;
}

float
Stats::Armor::getResist () const
{
    return m_resistance;
}

Stats::Armor::AP
Stats::Armor::getArmor () const
{
    return m_armor;
}
