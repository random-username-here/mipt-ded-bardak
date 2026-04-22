#pragma once
#include "modlib_mod.hpp"
#include <memory>
#include <vector>

namespace modlib {

struct Vec2i { int x = 0, y = 0; };

class Map;
class Tile;

class Unit {
protected:
    virtual Map *map() = 0;
    virtual Tile *tile() = 0; // map().at(pos())

    virtual int hp() const = 0;
    virtual Vec2i pos() const = 0;
    virtual void takeDamage(int d) = 0;
    virtual size_t id() = 0;

    virtual void move(Vec2i to);
    virtual void destroy();
};

class Tile {
public:
    virtual Vec2i pos() const;
    virtual const std::vector<Unit*> &units() = 0;
    virtual bool isWalkable() const = 0;
};

class Map : public Mod {
    friend class Unit;
    virtual Unit *addUnit(Vec2i pos, std::unique_ptr<Unit> &&u) = 0;
    virtual void moveUnit(Unit *u, Vec2i pos) = 0;
    virtual void removeUnit(Unit *u) = 0;
    size_t lastId = 0;
public:

    template<typename T, typename ...Args>
    T* spawn(Vec2i pos, Args... args) {
        return static_cast<T*>(addUnit(pos, std::make_unique<T>(this, pos, lastId++, args...)));
    }

    virtual Unit *byId(size_t id) = 0;

    virtual Vec2i size() const = 0;
    virtual Tile *at(Vec2i pos) = 0;
};


inline void Unit::move(Vec2i to) { map()->moveUnit(this, to); }
inline void Unit::destroy() { map()->removeUnit(this); }

};
