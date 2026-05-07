#pragma once

#include "Animator.hpp"
#include "AssetManager.hpp"
#include "Map.hpp"
#include "Timer.hpp"

#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

//TODO unify tile size between different persons and map
constexpr float kMapTilePixels = 40.0f;

namespace tile {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 16, 16};
    static constexpr modlib::Vec2f kSize = {kMapTilePixels, kMapTilePixels};
};

constexpr int kObjectLayer = std::numeric_limits<int>::min();
constexpr int kZ = std::numeric_limits<int>::min();

}

namespace assets {

template <typename TConfig>
static const modlib::SpriteAsset
Asset(std::string_view id, const std::string& file)
{
    return {
        .id = id,
        .file = file,
        .clip = TConfig::kClip,
        .size = TConfig::kSize,
    };
}

const auto Ground = Asset<tile::Config>("m.gnd",  ASSETS_DIR "/map/ground.png");
const auto Wall   = Asset<tile::Config>("m.wall", ASSETS_DIR "/map/spikes.png");

}

class MapAnimator {
    modlib::Level* m_map = nullptr;
    anim::AnimationManager* m_anim = nullptr;
    modlib::AssetManager* m_assets = nullptr;
    modlib::Timer* m_timer = nullptr;
    std::unordered_set<modlib::Tile*> m_subscribed_tiles;
    std::unordered_map<modlib::Tile*, anim::AnimatedObjectID> m_tile_objects;
    anim::SpriteSlotID m_tile_slot = 0;

	struct {
    	anim::AnimationID ground = anim::NO_ANIMATION;
    	anim::AnimationID wall = anim::NO_ANIMATION;
	} m_anims;

public:
    MapAnimator(
        modlib::Level* map,
        anim::AnimationManager* anim,
        modlib::AssetManager* assets,
        modlib::Timer* timer
    ) : m_map(map), m_anim(anim), m_assets(assets), m_timer(timer) {}

    void start() {
        m_tile_slot = m_anim->newSpriteSlot();
        registerAssets();
        buildAnimations();

		m_map->EvLevelLoaded.subscribe([this]() {
			subscribeAndAnimateAll();
		});

		m_timer->setTimer(
			1,
			[this]() {
				subscribeAndAnimateAll();
			},
			modlib::Timer::Stage::ON_UPDATE_DONE
		);
    }

private:
    void subscribeAndAnimateAll() {
        const modlib::Vec2i size = m_map->getSize();
        for (int x = 0; x < size.x; ++x) {
            for (int y = 0; y < size.y; ++y) {
                modlib::Tile* tile = m_map->getTile({x, y});
                subscribeTile(tile);
                animateTile(tile);
            }
        }
    }

    void subscribeTile(modlib::Tile* tile) {
        if (m_subscribed_tiles.find(tile) != m_subscribed_tiles.end()) {
            return;
        }

        tile->EvTileTypeChanged.subscribe(
            [this, tile](modlib::Tile::Type) {
                animateTile(tile);
            }
        );
        m_subscribed_tiles.insert(tile);
    }

    void animateTile(modlib::Tile* tile) {
        m_anim->play(
            tileObjectID(tile),
            pixelPosition(tile->getPos()),
            tile::kObjectLayer,
            animationFor(tile->getType())
        );
    }

    void buildAnimations() {
        m_anims.ground = buildTileAnimation(assets::Ground);
        m_anims.wall = buildTileAnimation(assets::Wall);
    }

    anim::AnimationID buildTileAnimation(const modlib::SpriteAsset& sprite) {
        auto* animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_tile_slot, sprite.id, tile::kZ);
        animation->finishBuild();
        return animation->id();
    }

    void registerAssets() {
        m_assets->registerSprite(assets::Ground);
        m_assets->registerSprite(assets::Wall);
    }

    anim::AnimationID animationFor(modlib::Tile::Type type) const {
        if (type == modlib::Tile::BasicTypes::WALL) {
            return m_anims.wall;
        }

        return m_anims.ground;
    }

    anim::AnimatedObjectID tileObjectID(modlib::Tile* tile) {
        auto it = m_tile_objects.find(tile);
        if (it != m_tile_objects.end()) {
            return it->second;
        }

        const anim::AnimatedObjectID id = m_anim->newObject();
        m_tile_objects.emplace(tile, id);
        return id;
    }

    static modlib::Vec2f pixelPosition(modlib::Vec2i cell) {
        return modlib::Vec2f( cell.x * kMapTilePixels, cell.y * kMapTilePixels );
    }
};
