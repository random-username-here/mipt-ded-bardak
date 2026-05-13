#pragma once

#include "Animator.hpp"
#include "AssetManager.hpp"
#include "Map.hpp"
#include "bullet_controller.hpp"

#include <array>
#include <cmath>
#include <string>
#include <string_view>

constexpr inline float kTankTilePixels = 16.0f;

namespace bullet_body {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 16, 16};
    static constexpr modlib::Vec2f kSize  = {kTankTilePixels, kTankTilePixels};
};

constexpr int kZ = 3;
constexpr int kObjectLayer = 3;
static constexpr float kMoveSeconds = 0.06f;

} // namespace bullet_body

namespace bullet_explode {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 32, 32};
    static constexpr modlib::Vec2f kSize  = {kTankTilePixels * 2.0f, kTankTilePixels * 2.0f};
};

constexpr int kZ = 5;
static constexpr float kFrameSeconds = 0.03f;

} // namespace bullet_explode

namespace bullet_assets {

inline modlib::SpriteAsset flySprite()
{
    return {
        .id = "t.b.fly",
        .file = ASSETS_DIR "/units/tank/bullet_fly.png",
        .clip = bullet_body::Config::kClip,
        .size = bullet_body::Config::kSize,
        .origin = {8, 8},
    };
}

inline modlib::SpriteAsset explodeSprite(std::string_view id, int col)
{
    return {
        .id = id,
        .file = ASSETS_DIR "/units/tank/bullet_explode.png",
        .clip = {static_cast<float>(col * 32), 0, 32, 32},
        .size = bullet_explode::Config::kSize,
        .origin = {16, 16},
    };
}

static const std::array<modlib::SpriteAsset, 6> Explode = {
    explodeSprite("t.b.ex.1", 0),
    explodeSprite("t.b.ex.2", 1),
    explodeSprite("t.b.ex.3", 2),
    explodeSprite("t.b.ex.4", 3),
    explodeSprite("t.b.ex.5", 4),
    explodeSprite("t.b.ex.6", 5),
};

} // namespace bullet_assets

class BulletAnimator {
    BulletCtl *m_ctl = nullptr;
    anim::AnimationManager *m_anim = nullptr;
    modlib::AssetManager *m_assets = nullptr;

    anim::AnimatedObjectID m_object = anim::NO_ANIMATION_OBJECT;
    anim::AnimatedObjectID m_explosionObject = anim::NO_ANIMATION_OBJECT;

    anim::SpriteSlotID m_bodySlot = 0;
    anim::SpriteSlotID m_explosionSlot = 0;

    anim::AnimationID m_idleAnim = anim::NO_ANIMATION;
    anim::AnimationID m_moveAnim = anim::NO_ANIMATION;
    anim::AnimationID m_explodeAnim = anim::NO_ANIMATION;

public:
    BulletAnimator(BulletCtl *ctl, anim::AnimationManager *anim, modlib::AssetManager *assets)
        : m_ctl(ctl)
        , m_anim(anim)
        , m_assets(assets)
    {
        m_object = m_anim->newObject();
        m_explosionObject = m_anim->newObject();

        m_bodySlot = m_anim->newSpriteSlot();
        m_explosionSlot = m_anim->newSpriteSlot();

        registerAssets();
        buildAnimations();
        subscribeOnEvents();

        animateSpawn();
    }

private:
    void registerAssets()
    {
        m_assets->registerSprite(bullet_assets::flySprite());
        for (const auto &asset : bullet_assets::Explode) {
            m_assets->registerSprite(asset);
        }
    }

    void buildAnimations()
    {
        m_idleAnim = buildIdleAnimation();
        m_moveAnim = buildMoveAnimation();
        m_explodeAnim = buildExplodeAnimation();
    }

    void subscribeOnEvents()
    {
        m_ctl->bullet()->EvEntityMoved.subscribe([this](modlib::Vec2i delta) {
            animateMove(delta);
        });

        m_ctl->bullet()->EvDeath.subscribe([this]() {
            animateDeath();
        });
    }

    void animateSpawn()
    {
        m_anim->play(
            m_object,
            currentPixelPosition(),
            bullet_body::kObjectLayer,
            m_idleAnim
        );
    }

    void animateMove(modlib::Vec2i delta)
    {
        if (m_ctl->bullet()->getCurrentHP() <= 0) {
            return;
        }

        const modlib::Vec2f oldPosition = pixelPosition(m_ctl->bullet()->getPosition() - delta);
        m_anim->play(m_object, oldPosition, bullet_body::kObjectLayer, m_moveAnim);
    }

    void animateDeath()
    {
        auto *hide = m_anim->newAnimation();
        hide->addStep<anim::DelSpriteStep>(m_bodySlot);
        hide->finishBuild();

        m_anim->play(
            m_object,
            currentPixelPosition(),
            bullet_body::kObjectLayer - 1,
            hide->id()
        );

        m_anim->play(
            m_explosionObject,
            currentPixelPosition(),
            bullet_explode::kZ,
            m_explodeAnim
        );
    }

    anim::AnimationID buildIdleAnimation()
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, bullet_assets::flySprite().id, bullet_body::kZ);
        animation->addStep<anim::SetRotationStep>(m_bodySlot, rotationDeg(m_ctl->bullet()->dir()));
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID buildMoveAnimation()
    {
        const Vec2i delta = dirDelta(m_ctl->bullet()->dir());
        const modlib::Vec2f to(delta.x * kTankTilePixels, delta.y * kTankTilePixels);

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, bullet_assets::flySprite().id, bullet_body::kZ);
        animation->addStep<anim::SetRotationStep>(m_bodySlot, rotationDeg(m_ctl->bullet()->dir()));
        animation->addStep<anim::PosStep>(
            bullet_body::kMoveSeconds,
            bullet_body::kMoveSeconds,
            m_bodySlot,
            to,
            anim::easing::linear
        );
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID buildExplodeAnimation()
    {
        auto *animation = m_anim->newAnimation();

        for (const auto &frame : bullet_assets::Explode) {
            animation->addStep<anim::SetAssetStep>(m_explosionSlot, frame.id, bullet_explode::kZ);
            animation->addStep<anim::Step>(bullet_explode::kFrameSeconds, bullet_explode::kFrameSeconds);
        }
        animation->addStep<anim::DelSpriteStep>(m_explosionSlot);
        animation->finishBuild();
        return animation->id();
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
        return {1, 0};
    }

    modlib::Vec2f currentPixelPosition() const
    {
        return pixelPosition(m_ctl->bullet()->getPosition());
    }

    static modlib::Vec2f pixelPosition(modlib::Vec2i cell)
    {
        return modlib::Vec2f(cell.x * kTankTilePixels, cell.y * kTankTilePixels);
    }
};
