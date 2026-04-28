#pragma once
#include "modlib_mod.hpp"
#include <memory>
#include <vector>

namespace modlib {

struct Vec2i { int x = 0, y = 0; };

class Map;
class Tile;

class Unit {
public:
    virtual Map *map() = 0;
    virtual Tile *tile() = 0; 

    virtual uint64_t type() const = 0;
    virtual uint64_t teamId() const = 0;

    virtual int hp() const = 0;
    virtual Vec2i pos() const = 0;
    virtual void takeDamage(int d) = 0;
    virtual size_t id() = 0;

    virtual void move(Vec2i to);
    virtual void destroy();
    virtual ~Unit() = default;
};

class Tile {
public:
    enum class BasicType : uint64_t {
        Ground = 0,
        Wall = 1
    };

    virtual Vec2i pos() const = 0;
    virtual const std::vector<Unit*> &units() = 0;
    virtual uint64_t type() const = 0;

    ~Tile() = default;
};

class Map : public Mod {
    friend class Unit;
    virtual Unit *addUnit(Vec2i pos, std::unique_ptr<Unit> &&u) = 0;
    virtual void moveUnit(Unit *u, Vec2i pos) = 0;
    virtual void removeUnit(Unit *u) = 0;
    size_t lastId = 0;
public:
    virtual ~Map() = default;

    template<typename T, typename ...Args>
    T* spawn(Vec2i pos, Args... args) {
        return static_cast<T*>(addUnit(pos, std::make_unique<T>(this, pos, lastId++, args...)));
    }

    virtual void setTile(Vec2i pos, const uint64_t type) = 0;
    virtual bool loadFromFile(const std::string& path) = 0;

    virtual Unit *byId(size_t id) = 0;

    virtual Vec2i size() const = 0;
    virtual Tile *at(Vec2i pos) = 0;
};


inline void Unit::move(Vec2i to) { map()->moveUnit(this, to); }
inline void Unit::destroy() { map()->removeUnit(this); }

inline bool operator==(uint64_t lhs, modlib::Tile::BasicType rhs) {
    return lhs == static_cast<uint64_t>(rhs);
}

inline bool operator==(modlib::Tile::BasicType lhs, uint64_t rhs) {
    return static_cast<uint64_t>(lhs) == rhs;
}

};
