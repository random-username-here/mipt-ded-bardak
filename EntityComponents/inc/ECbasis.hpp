#pragma once

#include "binmsg.hpp"
#include "Event.hpp"

namespace EC
{
    
    class EntityBase
    {
    public:

        using ID = uint64_t;
        using Type = bmsg::Char64;

        EntityBase  ();
        ~EntityBase ();

                ID   getID   () const;
        virtual Type getType () const;

        Event<> EvEntityDeconstructed;

    private:
        ID   m_id;
        Type m_type;
    };


    namespace Stats
    {
        class Health : virtual public EntityBase
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
    }

    namespace Actions
    {
        class Combat : virtual public EntityBase
        {
        public:

        private:
            
        };
    }
}