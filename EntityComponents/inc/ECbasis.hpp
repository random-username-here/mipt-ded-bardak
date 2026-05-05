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

        Entity  ();
        ~Entity ();

                ID   getID   () const;
        virtual Type getType () const;

        Event<> EvEntityDeconstructed;

    private:
        ID   m_id;
        Type m_type;
    };


    namespace Stats
    {
        class Health : virtual public Entity
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

    namespace Social
    {
        class Group : virtual public Entity
        {
        public:
            using GID = uint64_t;
            
            void setGroupID (GID          gid);
            void setGroupID (bmsg::Char64 group);

            GID  getGroupID () const;

            Event<> EvGroupChanged;
        private:
            GID m_groupID = 0;
        };
    }
}