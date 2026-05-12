#include <cmath>
#include "ECbasis.hpp"

using namespace EC;

Stats::Armor::Armor (AP armor, AP resistance)
    : m_armor (armor), m_resistance (resistance) {}

float
Stats::Armor::calculateResist (AP armor) const
{
    float armorCoeff = .06 * armor;
    return (1 - armorCoeff) / (1 + std::abs (armorCoeff));
}

float
Stats::Armor::setArmor (AP armor)
{
    AP dif = armor - m_armor;

    m_armor      = armor;
    m_resistance = calculateResist (m_armor);

    EvArmorChanged.emit (dif);

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
