#pragma once

#include <cassert>
#include <cstddef>
#include "binmsg.hpp"
#include "Event.hpp"
#include "Map.hpp"

#include "Common.hpp"

namespace EC
{
    namespace Actions
    {
        class Movable
        : virtual public modlib::Map::Entity
        {
        public:
            Movable (modlib::Map::Tile* tile, modlib::Map::Coordinate speed);

            virtual modlib::Map::VecXY moveTo (modlib::Map::VecXY target);
            virtual modlib::Map::VecXY moveBy (modlib::Map::VecXY delta);

            modlib::Map::Coordinate getSpeed (                             ) const;
            void                    setSpeed (modlib::Map::Coordinate speed);
        
            Event<modlib::Map::Coordinate, modlib::Map::Coordinate> EvSpeedChanged;
        private:
            modlib::Map::Coordinate m_TPT;  //tiles per tick (speed)
        };
    }

    namespace Stats
    {
        class Armor     // TODO: this component should be a component of the item, not an entity
        {
        public:
            using AP = int;

            Armor (AP armor);

            virtual float               calculateResist (              AP      armor)  const;
            virtual Common::Damage::DMG calculateDamage (const Common::Damage& damage) const;

            float setArmor  (AP armor);

            AP    getArmor  () const;
            float getResist () const;

            Event<AP, float, AP, float> EvArmorChanged;

        private:
            AP    m_armor;
            float m_resistance;
        };

        class Health
        {
        public:
            using HP = size_t;

             Health (HP currentHP, HP maxHP);
            ~Health ();

            HP getCurrentHP () const;
            HP getMaxHP     () const;

                    void setMaxHP   (HP             maxHP);
            virtual HP   inflictDmg (Common::Damage damage);
            virtual HP   heal       (HP             healed);

            Event<HP, HP> EvMaxHPChanged;
            Event<HP> EvDamaged;
            Event<HP> EvHealed;
            Event<>   EvDeath;

        private:
            HP m_currentHP;
            HP m_maxHP;
        };
    }

    namespace Social
    {
        class Group
        {
        public:
            using GID = uint64_t;

            Group (GID groupID);

            void setGroupID (GID          gid);
            void setGroupID (bmsg::Char64 group);

            GID  getGroupID () const;

            Event<> EvGroupChanged;
        private:
            GID m_groupID = 0;
        };
    }
}
