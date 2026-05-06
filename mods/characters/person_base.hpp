#pragma once

#include "BmServerModule.hpp"
#include "Map.hpp"
#include "ECbasis.hpp"

#include <algorithm>
#include <cstdint>

struct PersonBase : public modlib::Entity, public EC::Stats::Armor,
                    public EC::Stats::Health, public EC::Stats::Attack,
                    public EC::Social::Group {
    const int MAX_HP = 100;

    modlib::BmClient* m_client = nullptr;

    uint64_t m_nextMoveTick = 0;
    uint64_t m_nextAttackTick = 0;
    bool m_destroyed = false;

    PersonBase(EC::Entity::Type type, modlib::Tile* tile, EC::Stats::Armor&& armor,
               EC::Stats::Health&& health, EC::Stats::Attack&& attack,
               EC::Social::Group&& group, modlib::BmClient* client)
        : modlib::Entity(type, tile), EC::Stats::Armor(armor), EC::Stats::Health(health),
          EC::Stats::Attack(attack), EC::Social::Group(group), m_client(client)
    {}

    virtual ~PersonBase() {}

    virtual void pickUp() {}

    bool isDestroyed() const { return m_destroyed; }

    virtual int attackDamage() const { return 10; }

    virtual bool canEnter(modlib::Tile* tile) const {
        return tile != nullptr && !(tile->getType() == modlib::Tile::BasicTypes::WALL);
    }

    virtual void move(modlib::Vec2i to) {
        modlib::Entity::setPosition(to);
    }

    virtual void destroy() {
        if (m_destroyed) {
            return;
        }

        m_destroyed = true;
        EC::Stats::Health::EvDeath.emit();
        modlib::Entity::EvEntityDeconstructed.emit();
    }
};
