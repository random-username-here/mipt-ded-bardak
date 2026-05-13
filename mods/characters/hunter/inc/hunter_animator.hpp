#pragma once

#include "Animator.hpp"
#include "AssetManager.hpp"
#include "Map.hpp"
#include "Vec2.hpp"
#include "hunter_controller.hpp"

#include <array>
#include <cmath>
#include <string>
#include <string_view>

constexpr float kHunterTilePixels = 16.0f;

namespace hunter_body {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 16, 16};
    static constexpr modlib::Vec2f kSize = {kHunterTilePixels, kHunterTilePixels};
};

constexpr int kZ = 0;
constexpr int kObjectLayer = 2;
static constexpr float kMoveSeconds   = 0.16f;
static constexpr float kChargeSeconds = 0.08f;

} // namespace hunter_body

namespace hunter_arrow {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 16, 16};
    static constexpr modlib::Vec2f kSize = {kHunterTilePixels, kHunterTilePixels};
};

constexpr int kZ = 1;
static constexpr float kFlySecondsPerTile = 0.045f;
static constexpr float kMinFlySeconds = 0.08f;

} // namespace hunter_arrow

namespace hunter_spawn {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 32, 32};
    static constexpr modlib::Vec2f kSize = {kHunterTilePixels * 2.0f, kHunterTilePixels * 2.0f};
};

constexpr int kZ = 4;
static constexpr float kFrameSeconds = 0.045f;

} // namespace hunter_spawn

namespace hunter_dead {

constexpr int kZ = -4;

} // namespace hunter_dead

namespace hunter_assets {

inline modlib::SpriteAsset sheetSprite(std::string_view id, const std::string &file, int col)
{
    return {
        .id   = id,
        .file = file,
        .clip = {static_cast<float>(col * 16), 0, 16, 16},
        .size = hunter_body::Config::kSize,
    };
}

static const std::array<modlib::SpriteAsset, 4> Idle = {
    sheetSprite("h.id.d", ASSETS_DIR "/units/hunter/hunter_idle.png", 0),
    sheetSprite("h.id.u", ASSETS_DIR "/units/hunter/hunter_idle.png", 1),
    sheetSprite("h.id.l", ASSETS_DIR "/units/hunter/hunter_idle.png", 2),
    sheetSprite("h.id.r", ASSETS_DIR "/units/hunter/hunter_idle.png", 3),
};

static const std::array<modlib::SpriteAsset, 4> Walk = {
    sheetSprite("h.w.d", ASSETS_DIR "/units/hunter/hunter_walk.png", 0),
    sheetSprite("h.w.u", ASSETS_DIR "/units/hunter/hunter_walk.png", 1),
    sheetSprite("h.w.l", ASSETS_DIR "/units/hunter/hunter_walk.png", 2),
    sheetSprite("h.w.r", ASSETS_DIR "/units/hunter/hunter_walk.png", 3),
};

static const std::array<modlib::SpriteAsset, 4> Charge = {
    sheetSprite("h.c.d", ASSETS_DIR "/units/hunter/hunter_charge.png", 0),
    sheetSprite("h.c.u", ASSETS_DIR "/units/hunter/hunter_charge.png", 1),
    sheetSprite("h.c.l", ASSETS_DIR "/units/hunter/hunter_charge.png", 2),
    sheetSprite("h.c.r", ASSETS_DIR "/units/hunter/hunter_charge.png", 3),
};

static const modlib::SpriteAsset Arrow = {
    .id     = "h.arrow",
    .file   = ASSETS_DIR "/units/hunter/projectile_volley.png",
    .clip   = hunter_arrow::Config::kClip,
    .size   = hunter_arrow::Config::kSize,
    .origin = {8, 8},
};

inline modlib::SpriteAsset spawnSprite(std::string_view id, int col)
{
    return {
        .id     = id,
        .file   = ASSETS_DIR "/units/hunter/anim_spawn.png",
        .clip   = {static_cast<float>(col * 32), 0, 32, 32},
        .size   = hunter_spawn::Config::kSize,
        .origin = {8, 8},
    };
}

static const modlib::SpriteAsset Dead = {
    .id   = "h.dead",
    .file = ASSETS_DIR "/units/hunter/hunter_dead.png",
    .clip = hunter_body::Config::kClip,
    .size = hunter_body::Config::kSize,
};

static const std::array<modlib::SpriteAsset, 6> Spawn = {
    spawnSprite("h.sp.1", 0),
    spawnSprite("h.sp.2", 1),
    spawnSprite("h.sp.3", 2),
    spawnSprite("h.sp.4", 3),
    spawnSprite("h.sp.5", 4),
    spawnSprite("h.sp.6", 5),
};

} // namespace hunter_assets

class HunterAnimator {
    HunterCtl              *m_ctl    = nullptr;
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
        std::array<anim::AnimationID, 4> volley{};
    } m_anims;

public:
    HunterAnimator(HunterCtl *ctl, anim::AnimationManager *anim, modlib::AssetManager *assets)
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
        m_assets->registerSprite(hunter_assets::Dead);
        for (const auto &asset : hunter_assets::Spawn) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : hunter_assets::Idle) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : hunter_assets::Walk) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : hunter_assets::Charge) {
            m_assets->registerSprite(asset);
        }
        m_assets->registerSprite(hunter_assets::Arrow);
    }

    void buildAnimations()
    {
        for (int i = 0; i < 4; ++i) {
            const auto dir = static_cast<HunterDir>(i);
            m_anims.idle [i] = buildIdleAnimation (hunter_assets::Idle  [i]);
            m_anims.move [i] = buildMoveAnimation (hunter_assets::Walk  [i], hunter_assets::Idle[i], dir);
            m_anims.volley[i] = buildShootAnimation(hunter_assets::Charge[i], hunter_assets::Idle[i], dirDelta(dir));
        }
    }

    void subscribeOnEvents()
    {
        m_ctl->hunter()->EvEntityMoved.subscribe([this](modlib::Vec2i delta) {
            animateMove(delta);
        });

        m_ctl->hunter()->EvAttack.subscribe([this](Hunter::Damage targetId) {
            animateShoot(targetId);
        });

        m_ctl->hunter()->EvDamaged.subscribe([this](EC::Stats::Health::HP) {
            animateHitFlash();
        });
        m_ctl->hunter()->EvDeath.subscribe([this]() {
            animateDeath();
        });
    }

    void animateSpawn()
    {
        auto *animation = m_anim->newAnimation();

        for (const auto &spawn : hunter_assets::Spawn) {
            animation->addStep<anim::SetAssetStep>(m_spawnSlot, spawn.id, hunter_spawn::kZ);
            animation->addStep<anim::Step>(hunter_spawn::kFrameSeconds, hunter_spawn::kFrameSeconds);
        }

        animation->addStep<anim::DelSpriteStep>(m_spawnSlot);
        animation->finishBuild();

        m_anim->play(
            m_spawnObject,
            currentPixelPosition(),
            hunter_body::kObjectLayer + 2,
            animation->id()
        );
    }

    void animateDeath()
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, hunter_assets::Dead.id, hunter_dead::kZ);
        animation->finishBuild();

        m_anim->play(
            m_object,
            currentPixelPosition(),
            hunter_body::kObjectLayer - 1,
            animation->id()
        );
    }

    void animateIdle()
    {
        m_anim->play(m_object, currentPixelPosition(), hunter_body::kObjectLayer, idleAnimation(m_ctl->hunter()->dir()));
    }

    void animateMove(modlib::Vec2i delta)
    {
        if (m_ctl->hunter()->getCurrentHP() <= 0) {
            return;
        }
        const modlib::Vec2f oldPosition = pixelPosition(m_ctl->hunter()->getPosition() - delta);
        m_anim->play(m_object, oldPosition, hunter_body::kObjectLayer, moveAnimation(hunterDirFromDelta(delta)));
    }

    void animateShoot(Hunter::Damage targetId)
    {
        if (m_ctl->hunter()->getCurrentHP() <= 0) {
            return;
        }
        const modlib::Vec2i delta = attackDelta(targetId);
        const HunterDir dir = hunterDirFromDelta(delta);
        const anim::AnimationID volley = buildShootAnimation(
            hunter_assets::Charge[dirIndex(dir)],
            hunter_assets::Idle[dirIndex(dir)],
            delta
        );

        m_anim->play(m_object, currentPixelPosition(), hunter_body::kObjectLayer, volley);
    }

    void animateHitFlash()
    {
        if (m_ctl->hunter()->getCurrentHP() <= 0) {
            return;
        }
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(
            m_flashSlot,
            hunter_assets::Idle[dirIndex(m_ctl->hunter()->dir())].id,
            hunter_body::kZ
        );
        animation->addStep<anim::SetWhiteStep>(m_flashSlot, true);
        animation->addStep<anim::Step>(0.08f, 0.08f);
        animation->addStep<anim::DelSpriteStep>(m_flashSlot);
        animation->finishBuild();

        m_anim->play(
            m_flashObject,
            currentPixelPosition(),
            hunter_body::kObjectLayer + 1,
            animation->id()
        );
    }

    anim::AnimationID buildIdleAnimation(const modlib::SpriteAsset &asset)
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, asset.id, hunter_body::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID buildMoveAnimation(
        const modlib::SpriteAsset &walk,
        const modlib::SpriteAsset &idle,
        HunterDir dir)
    {
        const modlib::Vec2i delta = dirDelta(dir);
        const modlib::Vec2f to(delta.x * kHunterTilePixels, delta.y * kHunterTilePixels);

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, walk.id, hunter_body::kZ);
        animation->addStep<anim::PosStep>(
            hunter_body::kMoveSeconds,
            hunter_body::kMoveSeconds,
            m_bodySlot,
            to,
            anim::easing::easeInOutQuart
        );
        animation->addStep<anim::SetAssetStep>(m_bodySlot, idle.id, hunter_body::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID buildShootAnimation(
        const modlib::SpriteAsset &charge,
        const modlib::SpriteAsset &idle,
        modlib::Vec2i targetDelta)
    {
        const modlib::Vec2f from(kHunterTilePixels * 0.5f, kHunterTilePixels * 0.5f);
        const modlib::Vec2f to(
            targetDelta.x * kHunterTilePixels + kHunterTilePixels * 0.5f,
            targetDelta.y * kHunterTilePixels + kHunterTilePixels * 0.5f
        );

        const float distanceTiles = std::sqrt(
            static_cast<float>(targetDelta.x * targetDelta.x + targetDelta.y * targetDelta.y)
        );
        const float flySeconds = std::max(
            hunter_arrow::kMinFlySeconds,
            distanceTiles * hunter_arrow::kFlySecondsPerTile
        );

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, charge.id, hunter_body::kZ);
        animation->addStep<anim::Step>(hunter_body::kChargeSeconds, hunter_body::kChargeSeconds);

        animation->addStep<anim::SetAssetStep>(m_arrowSlot, hunter_assets::Arrow.id, hunter_arrow::kZ);
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

        animation->addStep<anim::SetAssetStep>(m_bodySlot, idle.id, hunter_body::kZ);
        animation->finishBuild();

        return animation->id();
    }

    anim::AnimationID idleAnimation(HunterDir dir) const
    {
        return m_anims.idle[dirIndex(dir)];
    }

    anim::AnimationID moveAnimation(HunterDir dir) const
    {
        return m_anims.move[dirIndex(dir)];
    }

    static int dirIndex(HunterDir dir)
    {
        return static_cast<int>(dir);
    }

    static modlib::Vec2i dirDelta(HunterDir dir)
    {
        switch (dir) {
        case HunterDir::up:
            return {0, -1};
        case HunterDir::down:
            return {0, 1};
        case HunterDir::left:
            return {-1, 0};
        case HunterDir::right:
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

    modlib::Vec2i attackDelta(Hunter::Damage targetId) const
    {
        if (auto *target = m_ctl->map()->getEntity(static_cast<modlib::Entity::ID>(targetId))) {
            const modlib::Vec2i raw = target->getPosition() - m_ctl->hunter()->getPosition();
            if (combat_grid::inArcherRange(m_ctl->hunter()->getPosition(), target->getPosition())) {
                return raw;
            }
        }

        return dirDelta(m_ctl->hunter()->dir());
    }

    modlib::Vec2f currentPixelPosition() const
    {
        return pixelPosition(m_ctl->hunter()->getPosition());
    }

    static modlib::Vec2f pixelPosition(modlib::Vec2i cell)
    {
        return modlib::Vec2f(cell.x * kHunterTilePixels, cell.y * kHunterTilePixels);
    }
};
