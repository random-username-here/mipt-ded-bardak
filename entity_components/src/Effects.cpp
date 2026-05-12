#include "Effects.hpp"

EC::Effects::Effect::Effect(int64_t duration, modlib::Map::Entity *target, modlib::Timer *tm)
: m_duration(duration),
  m_target(target),
  m_tm(tm)
{
    tm->setTimer(m_duration, [this](){stopEffect();});
}

EC::Effects::Invisible::Invisible(int64_t duration, modlib::Map::Entity *target, modlib::Timer *tm)
: Effect(duration, target, tm) {}

bool EC::Effects::Invisible::stopEffect() {return true;}

EC::Effects::Poison::Poison(int64_t duration, uint64_t step, modlib::Map::Entity *target,
                            modlib::Timer *tm, uint64_t damage)
: Effect(duration, target, tm), m_damage(damage), m_step(step) {
    if ((m_health = dynamic_cast<EC::Stats::Health *>(target))) {
        tm->setTimer(step, [this](){attack();});
    }
}

void EC::Effects::Poison::attack() {
    m_health->EvDamaged.emit(m_damage);
    m_tm->setTimer(m_step, [this](){attack();});
}

bool EC::Effects::Poison::stopEffect() {return true;}
