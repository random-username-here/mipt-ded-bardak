#pragma once

#include "binmsg.hpp"
#include "Event.hpp"
#include "Map.hpp"

namespace EC
{

    class Entity
    {
    public:

        using ID = uint64_t;
        using Type = bmsg::Char64;

                 Entity (Type);
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

    namespace Items
    {
        using Type = bmsg::Char64;

        class Item
        {
        public:
            virtual Type   getType      () const = 0;
            virtual size_t getStackSize () const = 0;
        };

        class DroppedItem
        : public modlib::Entity
        {
            DroppedItem (std::unique_ptr<Item> item, modlib::Tile* tile);

            std::unique_ptr<Item> pickup ();

            const Item* getBindedItem () const;
        private:
            std::unique_ptr<Item> m_item;
        };


        class Droppable
        {
        public:
            DroppedItem* drop (modlib::Tile* where);

            Event<DroppedItem*> EvDropped;
        };

        class Durability
        {
        public:
            size_t getDurability (                 ) const;
            void   setDurability (size_t durability);

            Event<size_t>  EvRepaired;
            Event<>        EvBroken;
        private:
            size_t m_durability;
        };

        template<typename RV>
        class Interactive
        {
        public:
            virtual RV useOn (Entity* target) = 0;

            Event<Entity*> EvUsed;
        };

        class Inventory
        {
        public:
            void    addItem (                   std::unique_ptr<Item>                    item);
            void removeItem (std::unordered_map<std::unique_ptr<Item>, size_t>::iterator iterator);
            Item&   getItem (std::unordered_map<std::unique_ptr<Item>, size_t>::iterator iterator);

            Event<Item&> EvAddedItem;
            Event<Item&> EvRemovedItem;

            const std::unordered_map<std::unique_ptr<Item>, size_t>& getInventory () const;
        private:
                  std::unordered_map<std::unique_ptr<Item>, size_t> m_inventory;
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
