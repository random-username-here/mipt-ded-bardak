#pragma once

#include <cassert>

#include "Map.hpp"
#include "Timer.hpp"

#include "EC.hpp"

namespace EC
{
    namespace Effects
    {
        class Effect
        {
        public:
            Effect (int64_t duration, modlib::Map::Entity *target, modlib::Timer *tm);
            virtual ~Effect() = default;

            virtual bool stopEffect() = 0;

        protected:
            int64_t m_duration;
            modlib::Map::Entity *m_target;
            modlib::Timer *m_tm;
        };

        class Invisible : virtual public Effect
        {
        public:
            Invisible (int64_t duration, modlib::Map::Entity *target, modlib::Timer *tm);
            virtual ~Invisible() = default;

            virtual bool stopEffect();
        };

        class Poison : virtual public Effect
        {
        public:
            Poison (int64_t duration, uint64_t step, modlib::Map::Entity *target,
                    modlib::Timer *tm, uint64_t damage);
            virtual ~Poison() = default;

            virtual bool stopEffect();

            void attack();

        private:
            uint64_t m_damage;
            uint64_t m_step;
            EC::Stats::Health *m_health;
        };
    }
}
