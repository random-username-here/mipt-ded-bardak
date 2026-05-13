#pragma once

#include "Animator.hpp"
#include "AssetManager.hpp"
#include "Map.hpp"
#include "Vec2.hpp"
#include "paladin_controller.hpp"

#include <array>
#include <cmath>
#include <string>
#include <string_view>

constexpr float kPaladinTilePixels = 16.0f;

namespace paladin_body {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 16, 16};
    static constexpr modlib::Vec2f kSize = {kPaladinTilePixels, kPaladinTilePixels};
};

constexpr int kZ = 0;
constexpr int kObjectLayer = 2;
static constexpr float kMoveSeconds = 0.16f;
static constexpr float kAttackPoseSeconds = 0.08f;

} // namespace paladin_body

namespace paladin_smite {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 32, 32};
    static constexpr modlib::Vec2f kSize = {kPaladinTilePixels * 2.0f, kPaladinTilePixels * 2.0f};
};

constexpr int kZ = 1;
static constexpr float kFrameSeconds = 0.04f;

} // namespace paladin_smite

namespace paladin_spawn {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 32, 32};
    static constexpr modlib::Vec2f kSize = {kPaladinTilePixels * 2.0f, kPaladinTilePixels * 2.0f};
};

constexpr int kZ = 4;
static constexpr float kFrameSeconds = 0.045f;

} // namespace paladin_spawn

namespace paladin_dead {

constexpr int kZ = -4;

} // namespace paladin_dead

namespace paladin_assets {

inline modlib::SpriteAsset sheetSprite(std::string_view id, const std::string &file, int col)
{
    return {
        .id = id,
        .file = file,
        .clip = {static_cast<float>(col * 16), 0, 16, 16},
        .size = paladin_body::Config::kSize,
    };
}

inline modlib::SpriteAsset smiteSprite(std::string_view id, int col)
{
    return {
        .id = id,
        .file = ASSETS_DIR "/units/paladin/anim_smite.png",
        .clip = {static_cast<float>(col * 32), 0, 32, 32},
        .size = paladin_smite::Config::kSize,
        .origin = {16, 16},
    };
}

static const std::array<modlib::SpriteAsset, 4> Idle = {
    sheetSprite("p.id.d", ASSETS_DIR "/units/paladin/paladin_idle.png", 0),
    sheetSprite("p.id.u", ASSETS_DIR "/units/paladin/paladin_idle.png", 1),
    sheetSprite("p.id.l", ASSETS_DIR "/units/paladin/paladin_idle.png", 2),
    sheetSprite("p.id.r", ASSETS_DIR "/units/paladin/paladin_idle.png", 3),
};

static const std::array<modlib::SpriteAsset, 4> Walk = {
    sheetSprite("p.w.d", ASSETS_DIR "/units/paladin/paladin_walk.png", 0),
    sheetSprite("p.w.u", ASSETS_DIR "/units/paladin/paladin_walk.png", 1),
    sheetSprite("p.w.l", ASSETS_DIR "/units/paladin/paladin_walk.png", 2),
    sheetSprite("p.w.r", ASSETS_DIR "/units/paladin/paladin_walk.png", 3),
};

static const std::array<modlib::SpriteAsset, 4> Hit = {
    sheetSprite("p.h.d", ASSETS_DIR "/units/paladin/paladin_hit.png", 0),
    sheetSprite("p.h.u", ASSETS_DIR "/units/paladin/paladin_hit.png", 1),
    sheetSprite("p.h.l", ASSETS_DIR "/units/paladin/paladin_hit.png", 2),
    sheetSprite("p.h.r", ASSETS_DIR "/units/paladin/paladin_hit.png", 3),
};

static const std::array<modlib::SpriteAsset, 4> Slash = {
    smiteSprite("p.sl.1", 0),
    smiteSprite("p.sl.2", 1),
    smiteSprite("p.sl.3", 2),
    smiteSprite("p.sl.4", 3),
};

inline modlib::SpriteAsset spawnSprite(std::string_view id, int col)
{
    return {
        .id     = id,
        .file   = ASSETS_DIR "/units/paladin/anim_spawn.png",
        .clip   = {static_cast<float>(col * 32), 0, 32, 32},
        .size   = paladin_spawn::Config::kSize,
        .origin = {8, 8},
    };
}

static const modlib::SpriteAsset Dead = {
    .id   = "p.dead",
    .file = ASSETS_DIR "/units/paladin/paladin_dead.png",
    .clip = paladin_body::Config::kClip,
    .size = paladin_body::Config::kSize,
};

static const std::array<modlib::SpriteAsset, 6> Spawn = {
    spawnSprite("p.sp.1", 0),
    spawnSprite("p.sp.2", 1),
    spawnSprite("p.sp.3", 2),
    spawnSprite("p.sp.4", 3),
    spawnSprite("p.sp.5", 4),
    spawnSprite("p.sp.6", 5),
};

} // namespace paladin_assets

class PaladinAnimator {
    PaladinCtl              *m_ctl    = nullptr;
    anim::AnimationManager *m_anim   = nullptr;
    modlib::AssetManager   *m_assets = nullptr;
    anim::AnimatedObjectID  m_object      = anim::NO_ANIMATION_OBJECT;
    anim::AnimatedObjectID  m_flashObject = anim::NO_ANIMATION_OBJECT;
    anim::AnimatedObjectID  m_spawnObject = anim::NO_ANIMATION_OBJECT;
    anim::SpriteSlotID      m_spawnSlot = 0;
    anim::SpriteSlotID      m_flashSlot = 0;
    anim::SpriteSlotID      m_bodySlot  = 0;
    anim::SpriteSlotID      m_smiteSlot = 0;

    struct {
        std::array<anim::AnimationID, 4> idle{};
        std::array<anim::AnimationID, 4> move{};
        std::array<anim::AnimationID, 4> attack{};
    } m_anims;

public:
    PaladinAnimator(PaladinCtl *ctl, anim::AnimationManager *anim, modlib::AssetManager *assets)
        : m_ctl(ctl), m_anim(anim), m_assets(assets)
    {
        m_object      = m_anim->newObject();
        m_flashObject = m_anim->newObject();
        m_spawnObject = m_anim->newObject();

        m_bodySlot  = m_anim->newSpriteSlot();
        m_smiteSlot = m_anim->newSpriteSlot();
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
        m_assets->registerSprite(paladin_assets::Dead);
        for (const auto &asset : paladin_assets::Spawn) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : paladin_assets::Idle) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : paladin_assets::Walk) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : paladin_assets::Hit) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : paladin_assets::Slash) {
            m_assets->registerSprite(asset);
        }
    }

    void buildAnimations()
    {
        for (int i = 0; i < 4; ++i) {
            const auto dir = static_cast<PaladinDir>(i);
            m_anims.idle[i]   = buildIdleAnimation  (paladin_assets::Idle[i]);
            m_anims.move[i]   = buildMoveAnimation  (paladin_assets::Walk[i], paladin_assets::Idle[i], dir);
            m_anims.attack[i] = buildAttackAnimation(paladin_assets::Hit [i], paladin_assets::Idle[i], dir);
        }
    }

    void subscribeOnEvents()
    {
        m_ctl->paladin()->EvEntityMoved.subscribe([this](modlib::Vec2i delta) {
            animateMove(delta);
        });

        m_ctl->paladin()->EvAttack.subscribe([this](Paladin::Damage targetId) {
            animateAttack(targetId);
        });

        m_ctl->paladin()->EvDamaged.subscribe([this](EC::Stats::Health::HP) {
            animateHitFlash();
        });

        m_ctl->paladin()->EvDeath.subscribe([this]() {
            animateDeath();
        });
    }

    void animateSpawn()
    {
        auto *animation = m_anim->newAnimation();

        for (const auto &spawn : paladin_assets::Spawn) {
            animation->addStep<anim::SetAssetStep>(m_spawnSlot, spawn.id, paladin_spawn::kZ);
            animation->addStep<anim::Step>(paladin_spawn::kFrameSeconds, paladin_spawn::kFrameSeconds);
        }

        animation->addStep<anim::DelSpriteStep>(m_spawnSlot);
        animation->finishBuild();

        m_anim->play(
            m_spawnObject,
            currentPixelPosition(),
            paladin_body::kObjectLayer + 2,
            animation->id()
        );
    }

    void animateDeath()
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, paladin_assets::Dead.id, paladin_dead::kZ);
        animation->finishBuild();

        m_anim->play(
            m_object,
            currentPixelPosition(),
            paladin_body::kObjectLayer - 1,
            animation->id()
        );
    }

    void animateIdle()
    {
        m_anim->play(m_object, currentPixelPosition(), paladin_body::kObjectLayer, idleAnimation(m_ctl->paladin()->dir()));
    }

    void animateMove(modlib::Vec2i delta)
    {
        if (m_ctl->paladin()->getCurrentHP() <= 0) {
            return;
        }
        const modlib::Vec2f oldPosition = pixelPosition(m_ctl->paladin()->getPosition() - delta);
        m_anim->play(m_object, oldPosition, paladin_body::kObjectLayer, moveAnimation(paladinDirFromDelta(delta)));
    }

    void animateAttack(Paladin::Damage targetId)
    {
        if (m_ctl->paladin()->getCurrentHP() <= 0) {
            return;
        }
        const modlib::Vec2i delta = attackDelta(targetId);
        m_anim->play(m_object, currentPixelPosition(), paladin_body::kObjectLayer, attackAnimation(paladinDirFromDelta(delta)));
    }

    void animateHitFlash()
    {
        if (m_ctl->paladin()->getCurrentHP() <= 0) {
            return;
        }
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(
            m_flashSlot,
            paladin_assets::Idle[dirIndex(m_ctl->paladin()->dir())].id,
            paladin_body::kZ
        );
        animation->addStep<anim::SetWhiteStep>(m_flashSlot, true);
        animation->addStep<anim::Step>(0.08f, 0.08f);
        animation->addStep<anim::DelSpriteStep>(m_flashSlot);
        animation->finishBuild();

        m_anim->play(
            m_flashObject,
            currentPixelPosition(),
            paladin_body::kObjectLayer + 1,
            animation->id()
        );
    }

    anim::AnimationID buildIdleAnimation(const modlib::SpriteAsset &asset)
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, asset.id, paladin_body::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID buildMoveAnimation(
        const modlib::SpriteAsset &walk,
        const modlib::SpriteAsset &idle,
        PaladinDir dir)
    {
        const modlib::Vec2i delta = dirDelta(dir);
        const modlib::Vec2f to(delta.x * kPaladinTilePixels, delta.y * kPaladinTilePixels);

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, walk.id, paladin_body::kZ);
        animation->addStep<anim::PosStep>(
            paladin_body::kMoveSeconds,
            paladin_body::kMoveSeconds,
            m_bodySlot,
            to,
            anim::easing::easeInOutQuart
        );
        animation->addStep<anim::SetAssetStep>(m_bodySlot, idle.id, paladin_body::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID buildAttackAnimation(
        const modlib::SpriteAsset &hit,
        const modlib::SpriteAsset &idle,
        PaladinDir dir)
    {
        const modlib::Vec2i delta = dirDelta(dir);
        const modlib::Vec2f smiteOffset(
            delta.x * kPaladinTilePixels + kPaladinTilePixels * 0.5f,
            delta.y * kPaladinTilePixels + kPaladinTilePixels * 0.5f
        );

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, hit.id, paladin_body::kZ);
        animation->addStep<anim::Step>(paladin_body::kAttackPoseSeconds, paladin_body::kAttackPoseSeconds);
        animation->addStep<anim::SetPosStep>(m_smiteSlot, smiteOffset);
        animation->addStep<anim::SetRotationStep>(m_smiteSlot, smiteRotationDeg(dir));
        for (const auto &smite : paladin_assets::Slash) {
            animation->addStep<anim::SetAssetStep>(m_smiteSlot, smite.id, paladin_smite::kZ);
            animation->addStep<anim::Step>(paladin_smite::kFrameSeconds, paladin_smite::kFrameSeconds);
        }
        animation->addStep<anim::DelSpriteStep>(m_smiteSlot);
        animation->addStep<anim::SetAssetStep>(m_bodySlot, idle.id, paladin_body::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID idleAnimation(PaladinDir dir) const
    {
        return m_anims.idle[dirIndex(dir)];
    }

    anim::AnimationID moveAnimation(PaladinDir dir) const
    {
        return m_anims.move[dirIndex(dir)];
    }

    anim::AnimationID attackAnimation(PaladinDir dir) const
    {
        return m_anims.attack[dirIndex(dir)];
    }

    static int dirIndex(PaladinDir dir)
    {
        return static_cast<int>(dir);
    }

    static float smiteRotationDeg(PaladinDir dir)
    {
        switch (dir) {
        case PaladinDir::right:
            return 0.0f;
        case PaladinDir::down:
            return 90.0f;
        case PaladinDir::left:
            return 180.0f;
        case PaladinDir::up:
            return -90.0f;
        }
        return 0.0f;
    }

    static modlib::Vec2i dirDelta(PaladinDir dir)
    {
        switch (dir) {
        case PaladinDir::up:
            return {0, -1};
        case PaladinDir::down:
            return {0, 1};
        case PaladinDir::left:
            return {-1, 0};
        case PaladinDir::right:
            return {1, 0};
        }
        return {0, 1};
    }

    modlib::Vec2i attackDelta(Paladin::Damage targetId) const
    {
        if (auto *target = m_ctl->map()->getEntity(static_cast<modlib::Entity::ID>(targetId))) {
            const modlib::Vec2i raw = target->getPosition() - m_ctl->paladin()->getPosition();

            if (std::abs(raw.x) <= 1 && std::abs(raw.y) <= 1 && (std::abs(raw.x) + std::abs(raw.y)) > 0) {
                if (raw.x != 0) {
                    return {raw.x, 0};
                }

                return {0, raw.y};
            }
        }

        return dirDelta(m_ctl->paladin()->dir());
    }

    modlib::Vec2f currentPixelPosition() const
    {
        return pixelPosition(m_ctl->paladin()->getPosition());
    }

    static modlib::Vec2f pixelPosition(modlib::Vec2i cell)
    {
        return modlib::Vec2f(cell.x * kPaladinTilePixels, cell.y * kPaladinTilePixels);
    }
};
