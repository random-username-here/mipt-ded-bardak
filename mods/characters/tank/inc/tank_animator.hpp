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

constexpr float kKnightTilePixels = 16.0f;

namespace knight_body {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 16, 16};
    static constexpr modlib::Vec2f kSize = {kKnightTilePixels, kKnightTilePixels};
};

constexpr int kZ = 0;
constexpr int kObjectLayer = 2;
static constexpr float kMoveSeconds = 0.16f;
static constexpr float kAttackPoseSeconds = 0.08f;

} // namespace knight_body

namespace knight_slash {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 32, 32};
    static constexpr modlib::Vec2f kSize = {kKnightTilePixels * 2.0f, kKnightTilePixels * 2.0f};
};

constexpr int kZ = 1;
static constexpr float kFrameSeconds = 0.04f;

} // namespace knight_slash

namespace knight_spawn {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 32, 32};
    static constexpr modlib::Vec2f kSize = {kKnightTilePixels * 2.0f, kKnightTilePixels * 2.0f};
};

constexpr int kZ = 4;
static constexpr float kFrameSeconds = 0.045f;

} // namespace knight_spawn

namespace knight_dead {

constexpr int kZ = -4;

} // namespace knight_dead

namespace knight_assets {

inline modlib::SpriteAsset sheetSprite(std::string_view id, const std::string &file, int col)
{
    return {
        .id = id,
        .file = file,
        .clip = {static_cast<float>(col * 16), 0, 16, 16},
        .size = knight_body::Config::kSize,
    };
}

inline modlib::SpriteAsset slashSprite(std::string_view id, int col)
{
    return {
        .id = id,
        .file = ASSETS_DIR "/units/knight/anim_slash.png",
        .clip = {static_cast<float>(col * 32), 0, 32, 32},
        .size = knight_slash::Config::kSize,
        .origin = {16, 16},
    };
}

static const std::array<modlib::SpriteAsset, 4> Idle = {
    sheetSprite("k.id.d", ASSETS_DIR "/units/knight/knight_idle.png", 0),
    sheetSprite("k.id.u", ASSETS_DIR "/units/knight/knight_idle.png", 1),
    sheetSprite("k.id.l", ASSETS_DIR "/units/knight/knight_idle.png", 2),
    sheetSprite("k.id.r", ASSETS_DIR "/units/knight/knight_idle.png", 3),
};

static const std::array<modlib::SpriteAsset, 4> Walk = {
    sheetSprite("k.w.d", ASSETS_DIR "/units/knight/knight_walk.png", 0),
    sheetSprite("k.w.u", ASSETS_DIR "/units/knight/knight_walk.png", 1),
    sheetSprite("k.w.l", ASSETS_DIR "/units/knight/knight_walk.png", 2),
    sheetSprite("k.w.r", ASSETS_DIR "/units/knight/knight_walk.png", 3),
};

static const std::array<modlib::SpriteAsset, 4> Hit = {
    sheetSprite("k.h.d", ASSETS_DIR "/units/knight/knight_hit.png", 0),
    sheetSprite("k.h.u", ASSETS_DIR "/units/knight/knight_hit.png", 1),
    sheetSprite("k.h.l", ASSETS_DIR "/units/knight/knight_hit.png", 2),
    sheetSprite("k.h.r", ASSETS_DIR "/units/knight/knight_hit.png", 3),
};

static const std::array<modlib::SpriteAsset, 4> Slash = {
    slashSprite("k.sl.1", 0),
    slashSprite("k.sl.2", 1),
    slashSprite("k.sl.3", 2),
    slashSprite("k.sl.4", 3),
};

inline modlib::SpriteAsset spawnSprite(std::string_view id, int col)
{
    return {
        .id     = id,
        .file   = ASSETS_DIR "/units/knight/anim_spawn.png",
        .clip   = {static_cast<float>(col * 32), 0, 32, 32},
        .size   = knight_spawn::Config::kSize,
        .origin = {8, 8},
    };
}

static const modlib::SpriteAsset Dead = {
    .id   = "k.dead",
    .file = ASSETS_DIR "/units/knight/knight_dead.png",
    .clip = knight_body::Config::kClip,
    .size = knight_body::Config::kSize,
};

static const std::array<modlib::SpriteAsset, 6> Spawn = {
    spawnSprite("k.sp.1", 0),
    spawnSprite("k.sp.2", 1),
    spawnSprite("k.sp.3", 2),
    spawnSprite("k.sp.4", 3),
    spawnSprite("k.sp.5", 4),
    spawnSprite("k.sp.6", 5),
};

} // namespace knight_assets

class TankAnimator {
    TankCtl *m_ctl    = nullptr;
    anim::AnimationManager *m_anim   = nullptr;
    modlib::AssetManager   *m_assets = nullptr;
    anim::AnimatedObjectID  m_object      = anim::NO_ANIMATION_OBJECT;
    anim::AnimatedObjectID  m_flashObject = anim::NO_ANIMATION_OBJECT;
    anim::AnimatedObjectID  m_spawnObject = anim::NO_ANIMATION_OBJECT;
    anim::SpriteSlotID      m_spawnSlot = 0;
    anim::SpriteSlotID      m_flashSlot = 0;
    anim::SpriteSlotID      m_bodySlot  = 0;
    anim::SpriteSlotID      m_slashSlot = 0;

    struct {
        std::array<anim::AnimationID, 4> idle{};
        std::array<anim::AnimationID, 4> move{};
        std::array<anim::AnimationID, 4> attack{};
    } m_anims;

public:
    TankAnimator(TankCtl *ctl, anim::AnimationManager *anim, modlib::AssetManager *assets)
        : m_ctl(ctl), m_anim(anim), m_assets(assets)
    {
        m_object      = m_anim->newObject();
        m_flashObject = m_anim->newObject();
        m_spawnObject = m_anim->newObject();

        m_bodySlot  = m_anim->newSpriteSlot();
        m_slashSlot = m_anim->newSpriteSlot();
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
        m_assets->registerSprite(knight_assets::Dead);
        for (const auto &asset : knight_assets::Spawn) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : knight_assets::Idle) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : knight_assets::Walk) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : knight_assets::Hit) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : knight_assets::Slash) {
            m_assets->registerSprite(asset);
        }
    }

    void buildAnimations()
    {
        for (int i = 0; i < 4; ++i) {
            const auto dir = static_cast<TankDir>(i);
            m_anims.idle[i]   = buildIdleAnimation  (knight_assets::Idle[i]);
            m_anims.move[i]   = buildMoveAnimation  (knight_assets::Walk[i], knight_assets::Idle[i], dir);
            m_anims.attack[i] = buildAttackAnimation(knight_assets::Hit [i], knight_assets::Idle[i], dir);
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

        m_ctl->tank()->EvAttack.subscribe([this](Tank::Damage targetId) {
            animateAttack(targetId);
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

        for (const auto &spawn : knight_assets::Spawn) {
            animation->addStep<anim::SetAssetStep>(m_spawnSlot, spawn.id, knight_spawn::kZ);
            animation->addStep<anim::Step>(knight_spawn::kFrameSeconds, knight_spawn::kFrameSeconds);
        }

        animation->addStep<anim::DelSpriteStep>(m_spawnSlot);
        animation->finishBuild();

        m_anim->play(
            m_spawnObject,
            currentPixelPosition(),
            knight_body::kObjectLayer + 2,
            animation->id()
        );
    }

    void animateDeath()
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, knight_assets::Dead.id, knight_dead::kZ);
        animation->finishBuild();

        m_anim->play(
            m_object,
            currentPixelPosition(),
            knight_body::kObjectLayer - 1,
            animation->id()
        );
    }

    void animateIdle()
    {
        m_anim->play(m_object, currentPixelPosition(), knight_body::kObjectLayer, idleAnimation(m_ctl->tank()->dir()));
    }

    void animateMove(modlib::Vec2i delta)
    {
        if (m_ctl->tank()->getCurrentHP() <= 0) {
            return;
        }
        const modlib::Vec2f oldPosition = pixelPosition(m_ctl->tank()->getPosition() - delta);
        m_anim->play(m_object, oldPosition, knight_body::kObjectLayer, moveAnimation(tankDirFromDelta(delta)));
    }

    void animateAttack(Tank::Damage targetId)
    {
        if (m_ctl->tank()->getCurrentHP() <= 0) {
            return;
        }
        const modlib::Vec2i delta = attackDelta(targetId);
        m_anim->play(m_object, currentPixelPosition(), knight_body::kObjectLayer, attackAnimation(tankDirFromDelta(delta)));
    }

    void animateHitFlash()
    {
        if (m_ctl->tank()->getCurrentHP() <= 0) {
            return;
        }
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(
            m_flashSlot,
            knight_assets::Idle[dirIndex(m_ctl->tank()->dir())].id,
            knight_body::kZ
        );
        animation->addStep<anim::SetWhiteStep>(m_flashSlot, true);
        animation->addStep<anim::Step>(0.08f, 0.08f);
        animation->addStep<anim::DelSpriteStep>(m_flashSlot);
        animation->finishBuild();

        m_anim->play(
            m_flashObject,
            currentPixelPosition(),
            knight_body::kObjectLayer + 1,
            animation->id()
        );
    }

    anim::AnimationID buildIdleAnimation(const modlib::SpriteAsset &asset)
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, asset.id, knight_body::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID buildMoveAnimation(
        const modlib::SpriteAsset &walk,
        const modlib::SpriteAsset &idle,
        TankDir dir)
    {
        const modlib::Vec2i delta = dirDelta(dir);
        const modlib::Vec2f to(delta.x * kKnightTilePixels, delta.y * kKnightTilePixels);

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, walk.id, knight_body::kZ);
        animation->addStep<anim::PosStep>(
            knight_body::kMoveSeconds,
            knight_body::kMoveSeconds,
            m_bodySlot,
            to,
            anim::easing::easeInOutQuart
        );
        animation->addStep<anim::SetAssetStep>(m_bodySlot, idle.id, knight_body::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID buildAttackAnimation(
        const modlib::SpriteAsset &hit,
        const modlib::SpriteAsset &idle,
        TankDir dir)
    {
        const modlib::Vec2i delta = dirDelta(dir);
        const modlib::Vec2f slashOffset(
            delta.x * kKnightTilePixels + kKnightTilePixels * 0.5f,
            delta.y * kKnightTilePixels + kKnightTilePixels * 0.5f
        );

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, hit.id, knight_body::kZ);
        animation->addStep<anim::Step>(knight_body::kAttackPoseSeconds, knight_body::kAttackPoseSeconds);
        animation->addStep<anim::SetPosStep>(m_slashSlot, slashOffset);
        animation->addStep<anim::SetRotationStep>(m_slashSlot, slashRotationDeg(dir));
        for (const auto &slash : knight_assets::Slash) {
            animation->addStep<anim::SetAssetStep>(m_slashSlot, slash.id, knight_slash::kZ);
            animation->addStep<anim::Step>(knight_slash::kFrameSeconds, knight_slash::kFrameSeconds);
        }
        animation->addStep<anim::DelSpriteStep>(m_slashSlot);
        animation->addStep<anim::SetAssetStep>(m_bodySlot, idle.id, knight_body::kZ);
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

    anim::AnimationID attackAnimation(TankDir dir) const
    {
        return m_anims.attack[dirIndex(dir)];
    }

    static int dirIndex(TankDir dir)
    {
        return static_cast<int>(dir);
    }

    static float slashRotationDeg(TankDir dir)
    {
        switch (dir) {
        case TankDir::right:
            return 0.0f;
        case TankDir::down:
            return 90.0f;
        case TankDir::left:
            return 180.0f;
        case TankDir::up:
            return -90.0f;
        }
        return 0.0f;
    }

    static modlib::Vec2i dirDelta(TankDir dir)
    {
        switch (dir) {
        case TankDir::up:
            return {0, -1};
        case TankDir::down:
            return {0, 1};
        case TankDir::left:
            return {-1, 0};
        case TankDir::right:
            return {1, 0};
        }
        return {0, 1};
    }

    modlib::Vec2i attackDelta(Tank::Damage targetId) const
    {
        if (auto *target = m_ctl->map()->getEntity(static_cast<modlib::Entity::ID>(targetId))) {
            const modlib::Vec2i raw = target->getPosition() - m_ctl->tank()->getPosition();

            if (std::abs(raw.x) <= 1 && std::abs(raw.y) <= 1 && (std::abs(raw.x) + std::abs(raw.y)) > 0) {
                if (raw.x != 0) {
                    return {raw.x, 0};
                }

                return {0, raw.y};
            }
        }

        return dirDelta(m_ctl->tank()->dir());
    }

    modlib::Vec2f currentPixelPosition() const
    {
        return pixelPosition(m_ctl->tank()->getPosition());
    }

    static modlib::Vec2f pixelPosition(modlib::Vec2i cell)
    {
        return modlib::Vec2f(cell.x * kKnightTilePixels, cell.y * kKnightTilePixels);
    }
};
