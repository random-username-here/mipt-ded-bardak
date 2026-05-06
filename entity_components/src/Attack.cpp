#include <cmath>
#include "ECbasis.hpp"

using namespace EC;

Stats::Attack::Attack (Damage strength)
    : m_strength (strength) {}

Stats::Attack::Damage
Stats::Attack::getStrength () const
{
    return m_strength;
}

void
Stats::Attack::setStrength (Damage strength)
{
    m_strength = strength;
}
