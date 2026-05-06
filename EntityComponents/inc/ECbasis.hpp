#pragma once

#include <unordered_map>
#include <memory>
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
        class Armor
        {
        public:
            using AP = int;

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

            void setMaxHP (HP maxHP);
            HP   reduceHP (HP damage);
            HP   heal     (HP healed);

            Event<HP> EvDamaged;
            Event<HP> EvHealed;
            Event<>   EvDeath;

        private:
            HP m_currentHP;
            HP m_maxHP;
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
        : public Map::Entity
        {
            DroppedItem (std::unique_ptr<Item> item, Map::Tile* tile);

            std::unique_ptr<Item> pickup ();

            const Item* getBindedItem () const;
        private:
            std::unique_ptr<Item> m_item;
        };


        class Droppable
        {
        public:
            DroppedItem* drop (Map::Tile* where);

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
            void    addItem (Item* item);
            void removeItem (std::unordered_map<Item, size_t>::iterator iterator);
            void removeItem (Type                                       type);  // O(N)

            Event<Item&> EvAddedItem;
            Event<Item&> EvRemovedItem;

            const std::unordered_map<Item, size_t>& getInventory () const;
        private:
                  std::unordered_map<Item, size_t>  m_inventory;
        };

        
    }

    namespace Social
    {
        class Group
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