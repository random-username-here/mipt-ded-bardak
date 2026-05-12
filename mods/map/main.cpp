#include "Map.hpp"
#include "modlib_manager.hpp"

#include "map_animator.hpp"

#include <memory>

namespace modlib {

class DodoLevel : public Level {
    anim::AnimationManager *m_animator = nullptr;
    modlib::AssetManager *m_assets = nullptr;
    modlib::Timer *m_timer = nullptr;
    std::unique_ptr<MapAnimator> m_mapAnimator;

public:
    DodoLevel() {
        const int width = 20;
        const int height = 20;

        m_tileMap.reserve(width);

        for (int x = 0; x < width; ++x) {
            std::vector<Tile> column;
            column.reserve(height);

            for (int y = 0; y < height; ++y) {
                bool isWall = (x == 0 || x == width - 1 || y == 0 || y == height - 1);
                
                Tile::Type type = isWall ? Tile::BasicTypes::WALL : Tile::BasicTypes::EMPTY;

                column.emplace_back(*this, Vec2i(x, y), type);
                
                m_tileTypes[type]++;
            }
            
            m_tileMap.push_back(std::move(column));
        }
    }

    ~DodoLevel() = default;

    std::string_view id() const override { return "dodo6b.bardak.map"; } 
    std::string_view brief() const override { return "Provides tile grid with entities "; };
    ModVersion version() const override { return ModVersion(1, 0, 0); }

    void onResolveDeps(ModManager *mm) override {
        m_animator = mm->requireAnyOfType<anim::AnimationManager>("MapAnimator needs Animator");
        m_assets = mm->requireAnyOfType<modlib::AssetManager>("MapAnimator needs AssetManager");
        m_timer = mm->requireAnyOfType<modlib::Timer>("MapAnimator needs Timer");
    }

    void onDepsResolved(ModManager *) override {
        m_mapAnimator = std::make_unique<MapAnimator>(this, m_animator, m_assets, m_timer);
        m_mapAnimator->start();
    }
};

extern "C" Mod* modlib_create(ModManager*) { return new DodoLevel(); }

}; // namespace modlib
