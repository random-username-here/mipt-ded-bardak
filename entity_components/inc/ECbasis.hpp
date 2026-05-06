#pragma once

#include "binmsg.hpp"
#include "Event.hpp"

namespace EC
{

    class Entity
    {
    public:

        using ID = uint64_t;
        using Type = bmsg::Char64;

        Entity  (Type);
        virtual ~Entity ();

        ID   getID   () const;
        Type getType () const;

        Event<> EvEntityDeconstructed;

    protected:
        ID   m_id;
        Type m_type;
    };


    namespace Stats
    {
        class Armor
        {
        public:
            using AP = int;

            Armor (AP armor, AP resistance);

            virtual float calculateResist (AP armor) const;

            float setArmor  (AP armor);

            AP    getArmor  () const;
            float getResist () const;

            Event<AP> EvArmorChanged;

        private:
            AP    m_armor;
            float m_resistance;
        };

        class Health
        {
        public:
            using HP = size_t;

            Health (HP currentHP, HP maxHP);

            HP getCurrentHP () const;
            HP getMaxHP     () const;

            void setMaxHP   (HP maxHP);
            HP   inflictDmg (HP damage);
            HP   heal       (HP healed);

            Event<HP> EvDamaged;
            Event<HP> EvHealed;
            Event<>   EvDeath;

        private:
            HP m_currentHP;
            HP m_maxHP;
        };

        class Attack
        {
        public:
            using Damage = size_t;

            Attack (Damage strength);

            Damage getStrength () const;

            void setStrength (Damage strength);

            Event<Damage> EvAttack;

        private:
            Damage m_strength;
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
