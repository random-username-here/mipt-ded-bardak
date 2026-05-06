#pragma once

#include "BmServerModule.hpp"
#include "Map.hpp"
#include "ECbasis.hpp"

#include <algorithm>
#include <cstdint>

// struct PersonBase : virtual public modlib::Entity {
//     modlib::BmClient* m_client = nullptr;

//     bool m_destroyed = false;

//     PersonBase(EC::Entity::Type type, modlib::Tile* tile, modlib::BmClient* client)
//         : modlib::Entity(type, tile), m_client(client)
//     {}

//     virtual ~PersonBase() {}

//     virtual void pickUp() {}

//     bool isDestroyed() const { return m_destroyed; }

//     virtual int attackDamage() const { return 10; }

//     virtual bool canEnter(modlib::Tile* tile) const {
//         return tile != nullptr && !(tile->getType() == modlib::Tile::BasicTypes::WALL);
//     }

//     virtual void move(modlib::Vec2i to) {
//         modlib::Entity::setPosition(to);
//     }

//     virtual void destroy() {
//         if (m_destroyed) {
//             return;
//         }

//         m_destroyed = true;
//         EC::Stats::Health::EvDeath.emit();
//         modlib::Entity::EvEntityDeconstructed.emit();
//     }
// };
