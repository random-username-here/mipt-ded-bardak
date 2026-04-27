#pragma once
#include "modlib_mod.hpp"
#include "Map.hpp" 

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <unordered_map>
#include <memory>

namespace modlib {

class IDATile : public Tile {
    Vec2i pos_;
    std::vector<Unit *> units_;
    bool isWalkable_;

public:
    IDATile(const Vec2i pos, const bool isWalkable) 
        : pos_(pos), isWalkable_(isWalkable) {}
        
    ~IDATile() = default; 

    Vec2i pos() const override { return pos_; }
    const std::vector<Unit*> &units() override { return units_; }
    bool isWalkable() const override { return isWalkable_; }

    void addUnit(Unit *unit) {
        if (!unit) return;
        auto it = std::find(units_.begin(), units_.end(), unit);
        if (it == units_.end()) {
            units_.push_back(unit);
        }
    }

    void removeUnit(Unit* unit) {
        if (!unit) return;
        auto it = std::remove(units_.begin(), units_.end(), unit);
    
        if (it != units_.end()) {
            units_.erase(it, units_.end());
        }
    }
};

class IDAMapModule : public Map {
    std::unordered_map<size_t, std::unique_ptr<Unit>> units_;
    std::vector<std::unique_ptr<Unit>> destroyedUnits_;
    std::vector<std::vector<std::unique_ptr<IDATile>>> grid_;

    Unit *addUnit(Vec2i pos, std::unique_ptr<Unit> &&u) override {
        assert(u);
        Unit* ptr = u.get();
        IDATile *tile = static_cast<IDATile *>(u->tile());
        if (tile == nullptr) {
            std::cerr << id() << " failed to cast tile to `IDATile` in `addUnit`\n";
            return nullptr;
        }
    
        tile->addUnit(ptr);
        units_[u->id()] = std::move(u);
        return ptr;
    }

    void moveUnit(Unit *u, Vec2i pos) override {
        assert(u && u->tile());

        IDATile *tile = static_cast<IDATile *>(u->tile());
        if (tile == nullptr) {
            std::cerr << id() << " failed to cast tile to `IDATile` in `moveUnit`\n";
            return;
        }

        tile->removeUnit(u);

        IDATile *nextTile = static_cast<IDATile *>(at(pos));
        if (nextTile == nullptr) {
            std::cerr << id() << " failed to cast nextTile to `IDATile` in `moveUnit`\n";
            return;
        }

        if (nextTile) {
            nextTile->addUnit(u);
        }
    }

    void removeUnit(Unit *u) override {
        assert(u);
        assert(u->tile());
    
        IDATile *tile = static_cast<IDATile *>(u->tile());
        if (tile == nullptr) {
            std::cerr << id() << " failed to cast tile to `IDATile` in `addUnit`\n";
            return;
        }

        tile->removeUnit(u);        

        if (units_.find(u->id()) == units_.end()) return;
        destroyedUnits_.push_back(std::move(units_[u->id()]));
        units_.erase(u->id());
    }

    void initializeGrid() {
        const int width = 20;
        const int height = 20;
        grid_.resize(height);

        for (int y = 0; y < height; ++y) {
            grid_[y].reserve(width);
            for (int x = 0; x < width; ++x) {
                bool wall = (x == 0 || x == width - 1 || y == 0 || y == height - 1);
                auto tile = std::make_unique<IDATile>(Vec2i{x, y}, !wall);
                grid_[y].push_back(std::move(tile));
            }
        }
    }

public:
    IDAMapModule() {
        initializeGrid();
    }
    
    virtual ~IDAMapModule() = default;

    std::string_view id() const override { return "ida.bardak.map"; }
    std::string_view brief() const override { return "Provides tile grid with entities "; }
    ModVersion version() const override { return ModVersion(1, 0, 0); }

    Unit *byId(size_t id) override {
        auto it = units_.find(id);
        return (it != units_.end()) ? it->second.get() : nullptr;
    }

    Vec2i size() const override {
        if (grid_.empty()) return {0, 0};
        return Vec2i{static_cast<int>(grid_[0].size()), static_cast<int>(grid_.size())};
    }

    Tile *at(Vec2i pos) override {
        if (pos.y < 0 || pos.y >= (int)grid_.size()) return nullptr;
        if (pos.x < 0 || pos.x >= (int)grid_[pos.y].size()) return nullptr;
        return grid_[pos.y][pos.x].get();
    }
};

extern "C" Mod* modlib_create(ModManager*) { return new IDAMapModule(); }

} // namespace modlib