#include "IC.hpp"
#include "Map.hpp"
#include <memory>
using namespace EC::Items;


DroppedItem::DroppedItem (
    std::unique_ptr<Droppable> item,
    modlib::Map::Tile* tile
)
: modlib::Map::Entity (tile)
, m_item (std::move (item))
{}

void
DroppedItem::pickup (
    std::unique_ptr<Droppable>& owner
)
{
    if (m_item == nullptr) 
    {
        owner = nullptr;
        return;
    }

    owner = std::move (m_item);
    EvPickedUp.emit (
        owner.get ()
    );
}

Droppable*
DroppedItem::getBindedItem () const
{
    return m_item.get ();
}


DroppedItem*
Droppable::drop (
    std::unique_ptr<Droppable> ownership,
    modlib::Map::Tile* where
)
{
    DroppedItem* dropped = new DroppedItem (
        std::move (ownership),
        where
    );
    EvDropped.emit(
        dropped
    );

    return dropped;
}