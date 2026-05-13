#pragma once

#include "Animator.hpp"
#include "AssetManager.hpp"
#include "Map.hpp"
#include "Vec2.hpp"
#include "archer_controller.hpp"

#include <array>
#include <cmath>
#include <string>
#include <string_view>

constexpr float kArcherTilePixels = 16.0f;

namespace archer_body {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 16, 16};
    static constexpr modlib::Vec2f kSize = {kArcherTilePixels, kArcherTilePixels};
};

constexpr int kZ = 0;
constexpr int kObjectLayer = 2;
static constexpr float kMoveSeconds   = 0.16f;
static constexpr float kChargeSeconds = 0.08f;

} // namespace archer_body

namespace archer_arrow {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 16, 16};
    static constexpr modlib::Vec2f kSize = {kArcherTilePixels, kArcherTilePixels};
};

constexpr int kZ = 1;
static constexpr float kFlySecondsPerTile = 0.045f;
static constexpr float kMinFlySeconds = 0.08f;

} // namespace archer_arrow

namespace archer_spawn {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 32, 32};
    static constexpr modlib::Vec2f kSize = {kArcherTilePixels * 2.0f, kArcherTilePixels * 2.0f};
};

constexpr int kZ = 4;
static constexpr float kFrameSeconds = 0.045f;

} // namespace archer_spawn

namespace archer_dead {

constexpr int kZ = -4;

} // namespace archer_dead

namespace archer_assets {

inline modlib::SpriteAsset sheetSprite(std::string_view id, const std::string &file, int col)
{
    return {
        .id   = id,
        .file = file,
        .clip = {static_cast<float>(col * 16), 0, 16, 16},
        .size = archer_body::Config::kSize,
    };
}

static const std::array<modlib::SpriteAsset, 4> Idle = {
    sheetSprite("a.id.d", ASSETS_DIR "/units/archer/archer_idle.png", 0),
    sheetSprite("a.id.u", ASSETS_DIR "/units/archer/archer_idle.png", 1),
    sheetSprite("a.id.l", ASSETS_DIR "/units/archer/archer_idle.png", 2),
    sheetSprite("a.id.r", ASSETS_DIR "/units/archer/archer_idle.png", 3),
};

static const std::array<modlib::SpriteAsset, 4> Walk = {
    sheetSprite("a.w.d", ASSETS_DIR "/units/archer/archer_walk.png", 0),
    sheetSprite("a.w.u", ASSETS_DIR "/units/archer/archer_walk.png", 1),
    sheetSprite("a.w.l", ASSETS_DIR "/units/archer/archer_walk.png", 2),
    sheetSprite("a.w.r", ASSETS_DIR "/units/archer/archer_walk.png", 3),
};

static const std::array<modlib::SpriteAsset, 4> Charge = {
    sheetSprite("a.c.d", ASSETS_DIR "/units/archer/archer_charge.png", 0),
    sheetSprite("a.c.u", ASSETS_DIR "/units/archer/archer_charge.png", 1),
    sheetSprite("a.c.l", ASSETS_DIR "/units/archer/archer_charge.png", 2),
    sheetSprite("a.c.r", ASSETS_DIR "/units/archer/archer_charge.png", 3),
};

static const modlib::SpriteAsset Arrow = {
    .id     = "a.arrow",
    .file   = ASSETS_DIR "/units/archer/projectile_arrow.png",
    .clip   = archer_arrow::Config::kClip,
    .size   = archer_arrow::Config::kSize,
    .origin = {8, 8},
};

inline modlib::SpriteAsset spawnSprite(std::string_view id, int col)
{
    return {
        .id     = id,
        .file   = ASSETS_DIR "/units/archer/anim_spawn.png",
        .clip   = {static_cast<float>(col * 32), 0, 32, 32},
        .size   = archer_spawn::Config::kSize,
        .origin = {8, 8},
    };
}

static const modlib::SpriteAsset Dead = {
    .id   = "a.dead",
    .file = ASSETS_DIR "/units/archer/archer_dead.png",
    .clip = archer_body::Config::kClip,
    .size = archer_body::Config::kSize,
};

static const std::array<modlib::SpriteAsset, 6> Spawn = {
    spawnSprite("a.sp.1", 0),
    spawnSprite("a.sp.2", 1),
    spawnSprite("a.sp.3", 2),
    spawnSprite("a.sp.4", 3),
    spawnSprite("a.sp.5", 4),
    spawnSprite("a.sp.6", 5),
};

} // namespace archer_assets

class ArcherAnimator {
    ArcherCtl              *m_ctl    = nullptr;
    anim::AnimationManager *m_anim   = nullptr;
    modlib::AssetManager   *m_assets = nullptr;
    anim::AnimatedObjectID  m_object      = anim::NO_ANIMATION_OBJECT;
    anim::AnimatedObjectID  m_flashObject = anim::NO_ANIMATION_OBJECT;
    anim::AnimatedObjectID  m_spawnObject = anim::NO_ANIMATION_OBJECT;
    anim::SpriteSlotID      m_spawnSlot = 0;
    anim::SpriteSlotID      m_flashSlot = 0;
    anim::SpriteSlotID      m_bodySlot  = 0;
    anim::SpriteSlotID      m_arrowSlot = 0;

    struct {
        std::array<anim::AnimationID, 4> idle{};
        std::array<anim::AnimationID, 4> move{};
        std::array<anim::AnimationID, 4> shoot{};
    } m_anims;

public:
    ArcherAnimator(ArcherCtl *ctl, anim::AnimationManager *anim, modlib::AssetManager *assets)
        : m_ctl(ctl)
        , m_anim(anim)
        , m_assets(assets)
    {
        m_object      = m_anim->newObject();
        m_flashObject = m_anim->newObject();
        m_spawnObject = m_anim->newObject();

        m_bodySlot  = m_anim->newSpriteSlot();
        m_arrowSlot = m_anim->newSpriteSlot();
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
        m_assets->registerSprite(archer_assets::Dead);
        for (const auto &asset : archer_assets::Spawn) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : archer_assets::Idle) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : archer_assets::Walk) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : archer_assets::Charge) {
            m_assets->registerSprite(asset);
        }
        m_assets->registerSprite(archer_assets::Arrow);
    }

    void buildAnimations()
    {
        for (int i = 0; i < 4; ++i) {
            const auto dir = static_cast<ArcherDir>(i);
            m_anims.idle [i] = buildIdleAnimation (archer_assets::Idle  [i]);
            m_anims.move [i] = buildMoveAnimation (archer_assets::Walk  [i], archer_assets::Idle[i], dir);
            m_anims.shoot[i] = buildShootAnimation(archer_assets::Charge[i], archer_assets::Idle[i], dirDelta(dir));
        }
    }

    void subscribeOnEvents()
    {
        m_ctl->archer()->EvEntityMoved.subscribe([this](modlib::Vec2i delta) {
            animateMove(delta);
        });

        m_ctl->archer()->EvAttack.subscribe([this](Archer::Damage targetId) {
            animateShoot(targetId);
        });

        m_ctl->archer()->EvDamaged.subscribe([this](EC::Stats::Health::HP) {
            animateHitFlash();
        });
        m_ctl->archer()->EvDeath.subscribe([this]() {
            animateDeath();
        });
    }

    void animateSpawn()
    {
        auto *animation = m_anim->newAnimation();

        for (const auto &spawn : archer_assets::Spawn) {
            animation->addStep<anim::SetAssetStep>(m_spawnSlot, spawn.id, archer_spawn::kZ);
            animation->addStep<anim::Step>(archer_spawn::kFrameSeconds, archer_spawn::kFrameSeconds);
        }

        animation->addStep<anim::DelSpriteStep>(m_spawnSlot);
        animation->finishBuild();

        m_anim->play(
            m_spawnObject,
            currentPixelPosition(),
            archer_body::kObjectLayer + 2,
            animation->id()
        );
    }

    void animateDeath()
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, archer_assets::Dead.id, archer_dead::kZ);
        animation->finishBuild();

        m_anim->play(
            m_object,
            currentPixelPosition(),
            archer_body::kObjectLayer - 1,
            animation->id()
        );
    }

    void animateIdle()
    {
        m_anim->play(m_object, currentPixelPosition(), archer_body::kObjectLayer, idleAnimation(m_ctl->archer()->dir()));
    }

    void animateMove(modlib::Vec2i delta)
    {
        if (m_ctl->archer()->getCurrentHP() <= 0) {
            return;
        }
        const modlib::Vec2f oldPosition = pixelPosition(m_ctl->archer()->getPosition() - delta);
        m_anim->play(m_object, oldPosition, archer_body::kObjectLayer, moveAnimation(archerDirFromDelta(delta)));
    }

    void animateShoot(Archer::Damage targetId)
    {
        if (m_ctl->archer()->getCurrentHP() <= 0) {
            return;
        }
        const modlib::Vec2i delta = attackDelta(targetId);
        const ArcherDir dir = archerDirFromDelta(delta);
        const anim::AnimationID shoot = buildShootAnimation(
            archer_assets::Charge[dirIndex(dir)],
            archer_assets::Idle[dirIndex(dir)],
            delta
        );

        m_anim->play(m_object, currentPixelPosition(), archer_body::kObjectLayer, shoot);
    }

    void animateHitFlash()
    {
        if (m_ctl->archer()->getCurrentHP() <= 0) {
            return;
        }
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(
            m_flashSlot,
            archer_assets::Idle[dirIndex(m_ctl->archer()->dir())].id,
            archer_body::kZ
        );
        animation->addStep<anim::SetWhiteStep>(m_flashSlot, true);
        animation->addStep<anim::Step>(0.08f, 0.08f);
        animation->addStep<anim::DelSpriteStep>(m_flashSlot);
        animation->finishBuild();

        m_anim->play(
            m_flashObject,
            currentPixelPosition(),
            archer_body::kObjectLayer + 1,
            animation->id()
        );
    }

    anim::AnimationID buildIdleAnimation(const modlib::SpriteAsset &asset)
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, asset.id, archer_body::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID buildMoveAnimation(
        const modlib::SpriteAsset &walk,
        const modlib::SpriteAsset &idle,
        ArcherDir dir)
    {
        const modlib::Vec2i delta = dirDelta(dir);
        const modlib::Vec2f to(delta.x * kArcherTilePixels, delta.y * kArcherTilePixels);

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, walk.id, archer_body::kZ);
        animation->addStep<anim::PosStep>(
            archer_body::kMoveSeconds,
            archer_body::kMoveSeconds,
            m_bodySlot,
            to,
            anim::easing::easeInOutQuart
        );
        animation->addStep<anim::SetAssetStep>(m_bodySlot, idle.id, archer_body::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID buildShootAnimation(
        const modlib::SpriteAsset &charge,
        const modlib::SpriteAsset &idle,
        modlib::Vec2i targetDelta)
    {
        const modlib::Vec2f from(kArcherTilePixels * 0.5f, kArcherTilePixels * 0.5f);
        const modlib::Vec2f to(
            targetDelta.x * kArcherTilePixels + kArcherTilePixels * 0.5f,
            targetDelta.y * kArcherTilePixels + kArcherTilePixels * 0.5f
        );

        const float distanceTiles = std::sqrt(
            static_cast<float>(targetDelta.x * targetDelta.x + targetDelta.y * targetDelta.y)
        );
        const float flySeconds = std::max(
            archer_arrow::kMinFlySeconds,
            distanceTiles * archer_arrow::kFlySecondsPerTile
        );

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, charge.id, archer_body::kZ);
        animation->addStep<anim::Step>(archer_body::kChargeSeconds, archer_body::kChargeSeconds);

        animation->addStep<anim::SetAssetStep>(m_arrowSlot, archer_assets::Arrow.id, archer_arrow::kZ);
        animation->addStep<anim::SetPosStep>(m_arrowSlot, from);
        animation->addStep<anim::SetRotationStep>(m_arrowSlot, arrowRotationDeg(targetDelta));
        animation->addStep<anim::PosStep>(
            flySeconds,
            flySeconds,
            m_arrowSlot,
            to,
            anim::easing::linear
        );
        animation->addStep<anim::DelSpriteStep>(m_arrowSlot);

        animation->addStep<anim::SetAssetStep>(m_bodySlot, idle.id, archer_body::kZ);
        animation->finishBuild();

        return animation->id();
    }

    anim::AnimationID idleAnimation(ArcherDir dir) const
    {
        return m_anims.idle[dirIndex(dir)];
    }

    anim::AnimationID moveAnimation(ArcherDir dir) const
    {
        return m_anims.move[dirIndex(dir)];
    }

    static int dirIndex(ArcherDir dir)
    {
        return static_cast<int>(dir);
    }

    static modlib::Vec2i dirDelta(ArcherDir dir)
    {
        switch (dir) {
        case ArcherDir::up:
            return {0, -1};
        case ArcherDir::down:
            return {0, 1};
        case ArcherDir::left:
            return {-1, 0};
        case ArcherDir::right:
            return {1, 0};
        }

        return {0, 1};
    }

    static float arrowRotationDeg(modlib::Vec2i delta)
    {
        return std::atan2(
            static_cast<float>(delta.y),
            static_cast<float>(delta.x)
        ) * 180.0f / M_PI;
    }

    modlib::Vec2i attackDelta(Archer::Damage targetId) const
    {
        if (auto *target = m_ctl->map()->getEntity(static_cast<modlib::Entity::ID>(targetId))) {
            const modlib::Vec2i raw = target->getPosition() - m_ctl->archer()->getPosition();
            if (combat_grid::inArcherRange(m_ctl->archer()->getPosition(), target->getPosition())) {
                return raw;
            }
        }

        return dirDelta(m_ctl->archer()->dir());
    }

    modlib::Vec2f currentPixelPosition() const
    {
        return pixelPosition(m_ctl->archer()->getPosition());
    }

    static modlib::Vec2f pixelPosition(modlib::Vec2i cell)
    {
        return modlib::Vec2f(cell.x * kArcherTilePixels, cell.y * kArcherTilePixels);
    }
};
