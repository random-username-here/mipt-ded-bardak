#pragma once

#include "Animator.hpp"
#include "AssetManager.hpp"
#include "Map.hpp"
#include "Timer.hpp"

#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

class MapAnimator {
    static constexpr float kTilePixels = 40.0f;
    static constexpr anim::SpriteID kTileSprite = 0;
    static constexpr int kTileLayer = -100;

    static constexpr std::string_view kGroundSprite = "t.ground";
    static constexpr std::string_view kWallSprite = "t.wall";

    modlib::Level *m_map = nullptr;
    anim::AnimationManager *m_anim = nullptr;
    modlib::AssetManager *m_assets = nullptr;
    modlib::Timer *m_timer = nullptr;
    std::unordered_set<modlib::Tile *> m_subscribedTiles;

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
        registerAssets();

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

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetImageStep>(kTileSprite, spriteFor(tile->getType()));
        animation->addStep<anim::PosStep>(0.0f, 0.0f, kTileSprite, modlib::Vec2f(0.0f, 0.0f));
        animation->finishBuild();

        m_anim->play(tileObjectID(tile), pixelPosition(tile->getPos()), kTileLayer, animation->id());
    }

    void registerAssets() {
        if (!m_assets) return;

        registerSprite(kGroundSprite, "assets/ground.png");
        registerSprite(kWallSprite, "assets/spikes.png");
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

    static std::string_view spriteFor(modlib::Tile::Type type) {
        if (type == modlib::Tile::BasicTypes::WALL) {
            return kWallSprite;
        }

        return kGroundSprite;
    }

    static anim::AnimatedObjectID tileObjectID(modlib::Tile *tile) {
        return reinterpret_cast<anim::AnimatedObjectID>(tile);
    }

    static modlib::Vec2f pixelPosition(modlib::Vec2i cell) {
        return modlib::Vec2f(
            static_cast<float>(cell.x) * kTilePixels,
            static_cast<float>(cell.y) * kTilePixels
        );
    }
};
