#pragma once

#include "Animator.hpp"
#include "AssetManager.hpp"
#include "Map.hpp"
#include "Timer.hpp"

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

class MapAnimator {
    static constexpr float kTilePixels = 40.0f;
    static constexpr int kTileLayer = -100;
    static constexpr int kTileZ = -100;

    static constexpr std::string_view kGroundSprite = "t.ground";
    static constexpr std::string_view kWallSprite = "t.wall";

    modlib::Level *m_map = nullptr;
    anim::AnimationManager *m_anim = nullptr;
    modlib::AssetManager *m_assets = nullptr;
    modlib::Timer *m_timer = nullptr;
    std::unordered_set<modlib::Tile *> m_subscribedTiles;
    std::unordered_map<modlib::Tile *, anim::AnimatedObjectID> m_tileObjects;
    anim::SpriteSlotID m_tileSlot = 0;
    anim::AnimationID m_groundAnimation = anim::NO_ANIMATION;
    anim::AnimationID m_wallAnimation = anim::NO_ANIMATION;

public:
    MapAnimator(
        modlib::Level *map,
        anim::AnimationManager *anim,
        modlib::AssetManager *assets,
        modlib::Timer *timer
    )
        : m_map(map)
        , m_anim(anim)
        , m_assets(assets)
        , m_timer(timer)
    {}

    void start() {
        m_tileSlot = m_anim->newSpriteSlot();
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
                modlib::Tile *tile = m_map->getTile(modlib::Vec2i(x, y));
                if (!tile) continue;

                subscribeTile(tile);
                animateTile(tile);
            }
        }
    }

    void subscribeTile(modlib::Tile *tile) {
        if (!tile || m_subscribedTiles.find(tile) != m_subscribedTiles.end()) {
            return;
        }

        tile->EvTileTypeChanged.subscribe(
            [this, tile](modlib::Tile::Type) {
                animateTile(tile);
            }
        );
        m_subscribedTiles.insert(tile);
    }

    void animateTile(modlib::Tile *tile) {
        if (!tile || !m_anim) return;

        m_anim->play(
            tileObjectID(tile),
            pixelPosition(tile->getPos()),
            kTileLayer,
            animationFor(tile->getType())
        );
    }

    void buildAnimations() {
        m_groundAnimation = buildTileAnimation(kGroundSprite);
        m_wallAnimation = buildTileAnimation(kWallSprite);
    }

    anim::AnimationID buildTileAnimation(modlib::SpriteID sprite) {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_tileSlot, sprite, kTileZ);
        animation->finishBuild();
        return animation->id();
    }

    void registerAssets() {
        if (!m_assets) return;

        registerSprite(kGroundSprite, "assets/map/ground.png");
        registerSprite(kWallSprite, "assets/map/spikes.png");
    }

    void registerSprite(std::string_view id, std::string file) {
        const modlib::SpriteID spriteId(id);
        if (m_assets->sprite(spriteId)) return;

        modlib::SpriteAsset sprite;
        sprite.id = spriteId;
        sprite.file = std::move(file);
        sprite.clip = modlib::Rectf{0, 0, 16, 16};
        sprite.size = modlib::Vec2f{kTilePixels, kTilePixels};

        m_assets->registerSprite(sprite);
    }

    anim::AnimationID animationFor(modlib::Tile::Type type) const {
        if (type == modlib::Tile::BasicTypes::WALL) {
            return m_wallAnimation;
        }

        return m_groundAnimation;
    }

    anim::AnimatedObjectID tileObjectID(modlib::Tile *tile) {
        auto it = m_tileObjects.find(tile);
        if (it != m_tileObjects.end()) {
            return it->second;
        }

        const anim::AnimatedObjectID id = m_anim->newObject();
        m_tileObjects.emplace(tile, id);
        return id;
    }

    static modlib::Vec2f pixelPosition(modlib::Vec2i cell) {
        return modlib::Vec2f(
            static_cast<float>(cell.x) * kTilePixels,
            static_cast<float>(cell.y) * kTilePixels
        );
    }
};
