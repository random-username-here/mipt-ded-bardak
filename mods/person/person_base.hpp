#pragma once

#include "BmServerModule.hpp"
#include "Map.hpp"

#include <algorithm>
#include <cstdint>

struct PersonBase : public modlib::MOB {
    modlib::BmClient* m_client = nullptr;
    modlib::Map* m_map = nullptr;
    modlib::Vec2i m_pos {};
    size_t m_id = 0;
    int m_hp = 100;
    uint64_t m_nextMoveTick = 0;
    uint64_t m_nextAttackTick = 0;
    bool m_destroyed = false;

    PersonBase(modlib::Map* map, modlib::Vec2i pos, size_t id, modlib::BmClient* client)
        : m_client(client), m_map(map), m_pos(pos), m_id(id)
    {}

    modlib::Map* map() override { return m_map; }
    modlib::Tile* tile() override { return m_map->at(m_pos); }

    uint64_t type() const override { return 0; }
    uint64_t teamId() const override { return 0; }

    int hp() const override { return m_hp; }
    modlib::Vec2i pos() const override { return m_pos; }
    size_t id() override { return m_id; }
    bool destroyed() const { return m_destroyed; }

    virtual int attackDamage() const { return 10; }

    virtual bool canEnter(modlib::Tile* tile) const {
        return tile != nullptr && !(tile->type() == modlib::Tile::BasicType::Wall);
    }

    void move(modlib::Vec2i to) override {
        modlib::Unit::move(to);
        m_pos = to;
    }

    void takeDamage(int d) override {
        m_hp = std::max(0, m_hp - d);
        if (m_hp == 0) {
            destroy();
        }
    }

    void destroy() override {
        if (m_destroyed) {
            return;
        }

        m_destroyed = true;
        modlib::Unit::destroy();
    }
};
