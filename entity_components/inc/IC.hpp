#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include "binmsg.hpp"
#include "Event.hpp"
#include "Map.hpp"

namespace EC
{
    namespace Items
    {
        using Type = bmsg::Char64;

        class Item
        {
        public:
            virtual Type getType () const = 0;
        };

        class Durability
        {
        public:
             Durability (size_t maxDurability, size_t currentDurability);
            ~Durability ();

            size_t getCurrentDurability (                 ) const;
            size_t getMaxDurability     (                 ) const;
            
            void modifyDurability     (int    modifier  );
            void setCurrentDurability (size_t durability);
            void setMaxDurability     (size_t durability);

            Event<size_t, size_t> EvDurabilityChanged;
            Event<size_t, size_t> EvMaxDurabilityChanged;
            Event<>               EvBroken;
        private:
            size_t m_maxDurability;
            size_t m_durability;
        };

        template<typename RV>
        class Interactive
        {
        public:
            virtual RV useOn (modlib::Map::Entity* target) = 0;

            Event<modlib::Map::Entity*> EvUsed;
        };


        class Droppable;

        class DroppedItem
        : virtual public modlib::Map::Entity
        {
        public:
            DroppedItem (std::unique_ptr<Droppable> item, modlib::Map::Tile* tile);

            void pickup (std::unique_ptr<Droppable>& newOwner);

            Droppable* getBindedItem () const;

            Event<Droppable*> EvPickedUp;
        private:
            std::unique_ptr<Droppable> m_item = nullptr;
        };


        class Droppable
        : virtual public Item
        {
        public:
            DroppedItem* drop (std::unique_ptr<Droppable> ownership, modlib::Map::Tile* where);

            Event<DroppedItem*> EvDropped;
        };
    }

    namespace Containers
    {
        template<size_t Size=6>
        class BasicContainer
        {
        public:
            size_t
            getCapacity ()
            {
                return Size;
            }

            size_t
            addItem (
                std::unique_ptr<Items::Item> item
            )
            {
                if (item == nullptr)
                {
                    return Size;
                }

                for (size_t i = 0; i < Size; i++)
                {
                    if (m_inventory[i] == nullptr)
                    {
                        m_inventory[i] = std::move (item);
                        EvAddedItem.emit (
                            i,
                            m_inventory[i].get ()
                        );
                        return i;
                    }
                }

                return Size;
            }
            
            void
            removeItem (
                size_t slot,
                std::unique_ptr<Items::Item>* owner
            )
            {
                assert (slot < Size);

                if (m_inventory[slot] == nullptr)
                {
                    if (owner)
                    {
                        *owner = nullptr;
                    }
                    return;
                }

                if (owner)
                {
                    *owner = std::move (m_inventory[slot]);
                    EvRemovedItem.emit (
                        slot,
                        m_inventory[slot].get()
                    );
                }
                else
                {
                    m_inventory[slot].reset();
                    EvRemovedItem.emit (
                        slot,
                        nullptr
                    );
                }
            }

            Items::Item*
            getItem (
                size_t slot
            ) const
            {
                assert(slot < Size);
                return m_inventory[slot].get ();
            }

            Event<size_t, Items::Item*> EvAddedItem;
            Event<size_t, Items::Item*> EvRemovedItem;

            const std::array<std::unique_ptr<Items::Item>, Size>&
            getInventory () const
            {
                return m_inventory;
            }
        protected:
            std::array<std::unique_ptr<Items::Item>, Size> m_inventory;
        };

        
        template<size_t Size=6>
        class Inventory
        : public BasicContainer<Size>
        {
        public:
            Items::DroppedItem*
            dropItem (
                size_t slot,
                modlib::Map::Tile* where
            )
            {
                assert (where);

                const Items::Droppable* itemProbeCasted = dynamic_cast<const Items::Droppable*> (
                    BasicContainer<Size>::getItem (slot)
                );
                if (itemProbeCasted == nullptr)
                {
                    return nullptr;
                }

                std::unique_ptr<Items::Droppable> tempOwner;
                BasicContainer<Size>::removeItem (
                    slot,
                    tempOwner
                );

                Items::Droppable* itemDroppable = const_cast<Items::Droppable*> (itemProbeCasted);

                Items::DroppedItem* drop = itemDroppable->drop(
                    std::move (tempOwner), 
                    where
                );

                EvDroppedItem.emit (
                    slot,
                    where,
                    drop
                );

                return drop;
            }

            size_t
            pickupItem (
                Items::DroppedItem* droppedItem
            )
            {
                assert (droppedItem);

                size_t slot = 0;
                for (; slot < Size; slot++)
                {
                    if (BasicContainer<Size>::m_inventory[slot] == nullptr)
                    {
                        break;
                    }
                }
                if (slot == Size - 1 && BasicContainer<Size>::m_inventory[slot])
                {
                    return Size;
                }

                droppedItem->pickup(
                    BasicContainer<Size>::m_inventory[slot]
                );

                droppedItem->getTile ()->removeEntity (droppedItem);
                delete droppedItem;

                EvPickedUpItem.emit (slot);

                return slot;
            }

            Event<size_t, modlib::Map::Tile*, Items::DroppedItem*> EvDroppedItem;
            Event<size_t>                                          EvPickedUpItem;
        private:
            
        };
    }
}