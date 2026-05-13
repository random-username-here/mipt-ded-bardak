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

// TODO: unify tile size between different persons and map
constexpr float kMapTilePixels = 16.0f;

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

const auto Ground = Asset<tile::Config>("m.gnd",  ASSETS_DIR "/map/tile_grass.png");
const auto Grass1 = Asset<tile::Config>("m.g.1",  ASSETS_DIR "/map/tile_grass_1.png");
const auto Grass2 = Asset<tile::Config>("m.g.2",  ASSETS_DIR "/map/tile_grass_2.png");
const auto Grass3 = Asset<tile::Config>("m.g.3",  ASSETS_DIR "/map/tile_grass_3.png");
const auto Grass4 = Asset<tile::Config>("m.g.4",  ASSETS_DIR "/map/tile_grass_4.png");
const auto Grass5 = Asset<tile::Config>("m.g.5",  ASSETS_DIR "/map/tile_grass_5.png");
const auto Grass6 = Asset<tile::Config>("m.g.6",  ASSETS_DIR "/map/tile_grass_6.png");
const auto Wall   = Asset<tile::Config>("m.wall", ASSETS_DIR "/map/spikes.png");

}

namespace tile_types {

const auto Grass1 = modlib::Tile::Type("grass1");
const auto Grass2 = modlib::Tile::Type("grass2");
const auto Grass3 = modlib::Tile::Type("grass3");
const auto Grass4 = modlib::Tile::Type("grass4");
const auto Grass5 = modlib::Tile::Type("grass5");
const auto Grass6 = modlib::Tile::Type("grass6");

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
        anim::AnimationID grass1 = anim::NO_ANIMATION;
        anim::AnimationID grass2 = anim::NO_ANIMATION;
        anim::AnimationID grass3 = anim::NO_ANIMATION;
        anim::AnimationID grass4 = anim::NO_ANIMATION;
        anim::AnimationID grass5 = anim::NO_ANIMATION;
        anim::AnimationID grass6 = anim::NO_ANIMATION;
        anim::AnimationID wall   = anim::NO_ANIMATION;
        anim::AnimationID clear  = anim::NO_ANIMATION;
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
            clearOldTileObjects();
            m_subscribed_tiles.clear();
            m_tile_objects.clear();
            subscribeAndAnimateAll();
        });

		m_timer->setTimer(
			1,
			[this]() {
				subscribeAndAnimateAll();
			},
			modlib::Timer::Stage::ON_UPDATE_DONE
		);
        subscribeAndAnimateAll();
    }

private:
    void clearOldTileObjects()
    {
        for (const auto &[tile, object] : m_tile_objects) {
            (void)tile;
            m_anim->play(
                object,
                {0, 0},
                tile::kObjectLayer,
                m_anims.clear
            );
        }
    }

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
        m_anims.grass1 = buildTileAnimation(assets::Grass1);
        m_anims.grass2 = buildTileAnimation(assets::Grass2);
        m_anims.grass3 = buildTileAnimation(assets::Grass3);
        m_anims.grass4 = buildTileAnimation(assets::Grass4);
        m_anims.grass5 = buildTileAnimation(assets::Grass5);
        m_anims.grass6 = buildTileAnimation(assets::Grass6);
        m_anims.wall   = buildTileAnimation(assets::Wall);
        m_anims.clear  = buildClearAnimation();
    }

    anim::AnimationID buildClearAnimation()
    {
        auto* animation = m_anim->newAnimation();
        animation->addStep<anim::DelSpriteStep>(m_tile_slot);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID buildTileAnimation(const modlib::SpriteAsset& sprite) {
        auto* animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_tile_slot, sprite.id, tile::kZ);
        animation->finishBuild();
        return animation->id();
    }

    void registerAssets() {
        m_assets->registerSprite(assets::Ground);
        m_assets->registerSprite(assets::Grass1);
        m_assets->registerSprite(assets::Grass2);
        m_assets->registerSprite(assets::Grass3);
        m_assets->registerSprite(assets::Grass4);
        m_assets->registerSprite(assets::Grass5);
        m_assets->registerSprite(assets::Grass6);
        m_assets->registerSprite(assets::Wall);
    }

    anim::AnimationID animationFor(modlib::Tile::Type type) const {
        if (type == modlib::Tile::BasicTypes::WALL) {
            return m_anims.wall;
        }

        if (type == tile_types::Grass1) {
            return m_anims.grass1;
        }

        if (type == tile_types::Grass2) {
            return m_anims.grass2;
        }

        if (type == tile_types::Grass3) {
            return m_anims.grass3;
        }

        if (type == tile_types::Grass4) {
            return m_anims.grass4;
        }

        if (type == tile_types::Grass5) {
            return m_anims.grass5;
        }

        if (type == tile_types::Grass6) {
            return m_anims.grass6;
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
