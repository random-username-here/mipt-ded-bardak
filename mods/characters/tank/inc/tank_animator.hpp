#pragma once

#include "Animator.hpp"
#include "AssetManager.hpp"
#include "Map.hpp"
#include "Vec2.hpp"
#include "tank_controller.hpp"

#include <array>
#include <cmath>
#include <string>
#include <string_view>

// constexpr inline float kTankTilePixels = 16.0f;

namespace tank_body {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 16, 16};
    static constexpr modlib::Vec2f kSize = {kTankTilePixels, kTankTilePixels};
};

constexpr int kZ = 0;
constexpr int kObjectLayer = 2;
static constexpr float kMoveSeconds = 0.16f;
static constexpr float kShootPoseSeconds = 0.08f;

} // namespace tank_body

namespace tank_spawn {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 32, 32};
    static constexpr modlib::Vec2f kSize = {kTankTilePixels * 2.0f, kTankTilePixels * 2.0f};
};

constexpr int kZ = 4;
static constexpr float kFrameSeconds = 0.045f;

} // namespace tank_spawn

namespace tank_dead {

constexpr int kZ = -4;

} // namespace tank_dead

namespace tank_assets {

inline modlib::SpriteAsset sheetSprite(std::string_view id, const std::string &file, int col)
{
    return {
        .id = id,
        .file = file,
        .clip = {static_cast<float>(col * 16), 0, 16, 16},
        .size = tank_body::Config::kSize,
    };
}

inline modlib::SpriteAsset spawnSprite(std::string_view id, int col)
{
    return {
        .id     = id,
        .file   = ASSETS_DIR "/units/tank/tank_spawn.png",
        .clip   = {static_cast<float>(col * 32), 0, 32, 32},
        .size   = tank_spawn::Config::kSize,
        .origin = {8, 8},
    };
}

static const std::array<modlib::SpriteAsset, 4> Idle = {
    sheetSprite("t.id.d", ASSETS_DIR "/units/tank/tank_idle.png", 0),
    sheetSprite("t.id.u", ASSETS_DIR "/units/tank/tank_idle.png", 1),
    sheetSprite("t.id.l", ASSETS_DIR "/units/tank/tank_idle.png", 2),
    sheetSprite("t.id.r", ASSETS_DIR "/units/tank/tank_idle.png", 3),
};

static const std::array<modlib::SpriteAsset, 4> Walk = {
    sheetSprite("t.w.d", ASSETS_DIR "/units/tank/tank_walk.png", 0),
    sheetSprite("t.w.u", ASSETS_DIR "/units/tank/tank_walk.png", 1),
    sheetSprite("t.w.l", ASSETS_DIR "/units/tank/tank_walk.png", 2),
    sheetSprite("t.w.r", ASSETS_DIR "/units/tank/tank_walk.png", 3),
};

static const std::array<modlib::SpriteAsset, 4> Shoot = {
    sheetSprite("t.s.d", ASSETS_DIR "/units/tank/tank_shoot.png", 0),
    sheetSprite("t.s.u", ASSETS_DIR "/units/tank/tank_shoot.png", 1),
    sheetSprite("t.s.l", ASSETS_DIR "/units/tank/tank_shoot.png", 2),
    sheetSprite("t.s.r", ASSETS_DIR "/units/tank/tank_shoot.png", 3),
};

static const modlib::SpriteAsset Dead = {
    .id   = "t.dead",
    .file = ASSETS_DIR "/units/tank/tank_dead.png",
    .clip = tank_body::Config::kClip,
    .size = tank_body::Config::kSize,
};

static const std::array<modlib::SpriteAsset, 6> Spawn = {
    spawnSprite("t.sp.1", 0),
    spawnSprite("t.sp.2", 1),
    spawnSprite("t.sp.3", 2),
    spawnSprite("t.sp.4", 3),
    spawnSprite("t.sp.5", 4),
    spawnSprite("t.sp.6", 5),
};

} // namespace tank_assets

class TankAnimator {
    TankCtl *m_ctl = nullptr;
    anim::AnimationManager *m_anim = nullptr;
    modlib::AssetManager *m_assets = nullptr;

    anim::AnimatedObjectID m_object = anim::NO_ANIMATION_OBJECT;
    anim::AnimatedObjectID m_flashObject = anim::NO_ANIMATION_OBJECT;
    anim::AnimatedObjectID m_spawnObject = anim::NO_ANIMATION_OBJECT;

    anim::SpriteSlotID m_bodySlot = 0;
    anim::SpriteSlotID m_flashSlot = 0;
    anim::SpriteSlotID m_spawnSlot = 0;

    struct {
        std::array<anim::AnimationID, 4> idle{};
        std::array<anim::AnimationID, 4> move{};
        std::array<anim::AnimationID, 4> shoot{};
    } m_anims;

public:
    TankAnimator(TankCtl *ctl, anim::AnimationManager *anim, modlib::AssetManager *assets)
        : m_ctl(ctl)
        , m_anim(anim)
        , m_assets(assets)
    {
        m_object      = m_anim->newObject();
        m_flashObject  = m_anim->newObject();
        m_spawnObject  = m_anim->newObject();

        m_bodySlot  = m_anim->newSpriteSlot();
        m_flashSlot = m_anim->newSpriteSlot();
        m_spawnSlot = m_anim->newSpriteSlot();

        registerAssets();
        buildAnimations();
        subscribeOnEvents();

        animateIdle();
        animateSpawn();
    }

private:
    void registerAssets()
    {
        m_assets->registerSprite(tank_assets::Dead);
        for (const auto &asset : tank_assets::Spawn) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : tank_assets::Idle) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : tank_assets::Walk) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : tank_assets::Shoot) {
            m_assets->registerSprite(asset);
        }
    }

    void buildAnimations()
    {
        for (int i = 0; i < 4; ++i) {
            const auto dir = static_cast<TankDir>(i);
            m_anims.idle[i]  = buildIdleAnimation(tank_assets::Idle[i]);
            m_anims.move[i]  = buildMoveAnimation(tank_assets::Walk[i], tank_assets::Idle[i], dir);
            m_anims.shoot[i] = buildShootAnimation(tank_assets::Shoot[i], tank_assets::Idle[i], dir);
        }
    }

    void subscribeOnEvents()
    {
        m_ctl->tank()->EvEntityMoved.subscribe([this](modlib::Vec2i delta) {
            animateMove(delta);
        });

        m_ctl->tank()->EvRotated.subscribe([this](TankDir /*dir*/) {
            animateIdle();
        });

        m_ctl->tank()->EvShoot.subscribe([this](TankDir dir) {
            animateShoot(dir);
        });

        m_ctl->tank()->EvDamaged.subscribe([this](EC::Stats::Health::HP) {
            animateHitFlash();
        });

        m_ctl->tank()->EvDeath.subscribe([this]() {
            animateDeath();
        });
    }

    void animateSpawn()
    {
        auto *animation = m_anim->newAnimation();

        for (const auto &spawn : tank_assets::Spawn) {
            animation->addStep<anim::SetAssetStep>(m_spawnSlot, spawn.id, tank_spawn::kZ);
            animation->addStep<anim::Step>(tank_spawn::kFrameSeconds, tank_spawn::kFrameSeconds);
        }

        animation->addStep<anim::DelSpriteStep>(m_spawnSlot);
        animation->finishBuild();

        m_anim->play(
            m_spawnObject,
            currentPixelPosition(),
            tank_body::kObjectLayer + 2,
            animation->id()
        );
    }

    void animateDeath()
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, tank_assets::Dead.id, tank_dead::kZ);
        animation->finishBuild();

        m_anim->play(
            m_object,
            currentPixelPosition(),
            tank_body::kObjectLayer - 1,
            animation->id()
        );
    }

    void animateIdle()
    {
        m_anim->play(
            m_object,
            currentPixelPosition(),
            tank_body::kObjectLayer,
            idleAnimation(m_ctl->tank()->dir())
        );
    }

    void animateMove(modlib::Vec2i delta)
    {
        if (m_ctl->tank()->getCurrentHP() <= 0) {
            return;
        }

        const modlib::Vec2f oldPosition = pixelPosition(m_ctl->tank()->getPosition() - delta);
        m_anim->play(
            m_object,
            oldPosition,
            tank_body::kObjectLayer,
            moveAnimation(tankDirFromDelta(delta))
        );
    }

    void animateShoot(TankDir dir)
    {
        if (m_ctl->tank()->getCurrentHP() <= 0) {
            return;
        }

        m_anim->play(
            m_object,
            currentPixelPosition(),
            tank_body::kObjectLayer,
            shootAnimation(dir)
        );
    }

    void animateHitFlash()
    {
        if (m_ctl->tank()->getCurrentHP() <= 0) {
            return;
        }

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(
            m_flashSlot,
            tank_assets::Idle[dirIndex(m_ctl->tank()->dir())].id,
            tank_body::kZ
        );
        animation->addStep<anim::SetWhiteStep>(m_flashSlot, true);
        animation->addStep<anim::Step>(0.08f, 0.08f);
        animation->addStep<anim::DelSpriteStep>(m_flashSlot);
        animation->finishBuild();

        m_anim->play(
            m_flashObject,
            currentPixelPosition(),
            tank_body::kObjectLayer + 1,
            animation->id()
        );
    }

    anim::AnimationID buildIdleAnimation(const modlib::SpriteAsset &asset)
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, asset.id, tank_body::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID buildMoveAnimation(
        const modlib::SpriteAsset &walk,
        const modlib::SpriteAsset &idle,
        TankDir dir)
    {
        const Vec2i delta = dirDelta(dir);
        const modlib::Vec2f to(delta.x * kTankTilePixels, delta.y * kTankTilePixels);

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, walk.id, tank_body::kZ);
        animation->addStep<anim::PosStep>(
            tank_body::kMoveSeconds,
            tank_body::kMoveSeconds,
            m_bodySlot,
            to,
            anim::easing::easeInOutQuart
        );
        animation->addStep<anim::SetAssetStep>(m_bodySlot, idle.id, tank_body::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID buildShootAnimation(
        const modlib::SpriteAsset &shoot,
        const modlib::SpriteAsset &idle,
        TankDir dir)
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, shoot.id, tank_body::kZ);
        animation->addStep<anim::SetRotationStep>(m_bodySlot, rotationDeg(dir));
        animation->addStep<anim::Step>(tank_body::kShootPoseSeconds, tank_body::kShootPoseSeconds);
        animation->addStep<anim::SetAssetStep>(m_bodySlot, idle.id, tank_body::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID idleAnimation(TankDir dir) const
    {
        return m_anims.idle[dirIndex(dir)];
    }

    anim::AnimationID moveAnimation(TankDir dir) const
    {
        return m_anims.move[dirIndex(dir)];
    }

    anim::AnimationID shootAnimation(TankDir dir) const
    {
        return m_anims.shoot[dirIndex(dir)];
    }

    static int dirIndex(TankDir dir)
    {
        return static_cast<int>(dir);
    }

    static float rotationDeg(TankDir dir)
    {
        switch (dir) {
        case TankDir::right: return 0.0f;
        case TankDir::down:  return 90.0f;
        case TankDir::left:  return 180.0f;
        case TankDir::up:    return -90.0f;
        }
        return 0.0f;
    }

    static Vec2i dirDelta(TankDir dir)
    {
        switch (dir) {
        case TankDir::up:    return {0, -1};
        case TankDir::down:  return {0, 1};
        case TankDir::left:  return {-1, 0};
        case TankDir::right: return {1, 0};
        }
        return {0, 1};
    }

    modlib::Vec2f currentPixelPosition() const
    {
        return pixelPosition(m_ctl->tank()->getPosition());
    }

    static modlib::Vec2f pixelPosition(modlib::Vec2i cell)
    {
        return modlib::Vec2f(cell.x * kTankTilePixels, cell.y * kTankTilePixels);
    }
};
