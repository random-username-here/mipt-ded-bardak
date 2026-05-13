#pragma once

#include "Animator.hpp"
#include "AssetManager.hpp"
#include "Map.hpp"
#include "Vec2.hpp"
#include "rogue_controller.hpp"

#include <array>
#include <cmath>
#include <string>
#include <string_view>

constexpr float kRogueTilePixels = 16.0f;

namespace rogue_body {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 16, 16};
    static constexpr modlib::Vec2f kSize = {kRogueTilePixels, kRogueTilePixels};
};

constexpr int kZ = 0;
constexpr int kObjectLayer = 2;
static constexpr float kMoveSeconds = 0.16f;
static constexpr float kAttackPoseSeconds = 0.05f;

} // namespace rogue_body

namespace rogue_slice {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 32, 32};
    static constexpr modlib::Vec2f kSize = {kRogueTilePixels * 2.0f, kRogueTilePixels * 2.0f};
};

constexpr int kZ = 1;
static constexpr float kFrameSeconds = 0.035f;

} // namespace rogue_slice

namespace rogue_assets {

inline modlib::SpriteAsset sheetSprite(std::string_view id, const std::string &file, int col)
{
    return {
        .id   = id,
        .file = file,
        .clip = {static_cast<float>(col * 16), 0, 16, 16},
        .size = rogue_body::Config::kSize,
    };
}

inline modlib::SpriteAsset sliceSprite(std::string_view id, int col)
{
    return {
        .id   = id,
        .file = ASSETS_DIR "/units/rogue/anim_slice.png",
        .clip = {static_cast<float>(col * 32), 0, 32, 32},
        .size = rogue_slice::Config::kSize,
        .origin = {16, 16},
    };
}

static const std::array<modlib::SpriteAsset, 4> Idle = {
    sheetSprite("r.id.d", ASSETS_DIR "/units/rogue/skull_idle.png", 0),
    sheetSprite("r.id.u", ASSETS_DIR "/units/rogue/skull_idle.png", 1),
    sheetSprite("r.id.l", ASSETS_DIR "/units/rogue/skull_idle.png", 2),
    sheetSprite("r.id.r", ASSETS_DIR "/units/rogue/skull_idle.png", 3),
};

static const std::array<modlib::SpriteAsset, 4> Walk = {
    sheetSprite("r.w.d", ASSETS_DIR "/units/rogue/skull_walk.png", 0),
    sheetSprite("r.w.u", ASSETS_DIR "/units/rogue/skull_walk.png", 1),
    sheetSprite("r.w.l", ASSETS_DIR "/units/rogue/skull_walk.png", 2),
    sheetSprite("r.w.r", ASSETS_DIR "/units/rogue/skull_walk.png", 3),
};

static const std::array<modlib::SpriteAsset, 4> Hit = {
    sheetSprite("r.h.d", ASSETS_DIR "/units/rogue/skull_hit.png", 0),
    sheetSprite("r.h.u", ASSETS_DIR "/units/rogue/skull_hit.png", 1),
    sheetSprite("r.h.l", ASSETS_DIR "/units/rogue/skull_hit.png", 2),
    sheetSprite("r.h.r", ASSETS_DIR "/units/rogue/skull_hit.png", 3),
};

static const std::array<modlib::SpriteAsset, 4> Slice = {
    sliceSprite("r.sl.1", 0),
    sliceSprite("r.sl.2", 1),
    sliceSprite("r.sl.3", 2),
    sliceSprite("r.sl.4", 3),
};

} // namespace rogue_assets

class RogueAnimator {
    RogueCtl               *m_ctl       = nullptr;
    anim::AnimationManager *m_anim      = nullptr;
    modlib::AssetManager   *m_assets    = nullptr;
    anim::AnimatedObjectID  m_object    = anim::NO_ANIMATION_OBJECT;
    anim::SpriteSlotID      m_bodySlot  = 0;
    anim::SpriteSlotID      m_sliceSlot = 0;

    struct {
        std::array<anim::AnimationID, 4> idle{};
        std::array<anim::AnimationID, 4> move{};
        std::array<anim::AnimationID, 4> attack{};
    } m_anims;

public:
    RogueAnimator(RogueCtl *ctl, anim::AnimationManager *anim, modlib::AssetManager *assets)
        : m_ctl(ctl)
        , m_anim(anim)
        , m_assets(assets)
    {
        m_object    = m_anim->newObject();
        m_bodySlot  = m_anim->newSpriteSlot();
        m_sliceSlot = m_anim->newSpriteSlot();

        registerAssets();
        buildAnimations();
        subscribeOnEvents();
        animateIdle();
    }

private:
    void registerAssets()
    {
        for (const auto &asset : rogue_assets::Idle) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : rogue_assets::Walk) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : rogue_assets::Hit) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : rogue_assets::Slice) {
            m_assets->registerSprite(asset);
        }
    }

    void buildAnimations()
    {
        for (int i = 0; i < 4; ++i) {
            const auto dir = static_cast<RogueDir>(i);
            m_anims.idle  [i] = buildIdleAnimation  (rogue_assets::Idle[i]);
            m_anims.move  [i] = buildMoveAnimation  (rogue_assets::Walk[i], rogue_assets::Idle[i], dir);
            m_anims.attack[i] = buildAttackAnimation(rogue_assets::Hit [i], rogue_assets::Idle[i], dir);
        }
    }

    void subscribeOnEvents()
    {
        m_ctl->rogue()->EvEntityMoved.subscribe([this](modlib::Vec2i delta) {
            animateMove(delta);
        });

        m_ctl->rogue()->EvAttack.subscribe([this](Rogue::Damage targetId) {
            animateAttack(targetId);
        });

        m_ctl->rogue()->EvDamaged.subscribe([this](EC::Stats::Health::HP) {
            animateHitFlash();
        });
    }

    void animateIdle()
    {
        m_anim->play(m_object, currentPixelPosition(), rogue_body::kObjectLayer, idleAnimation(m_ctl->rogue()->dir()));
    }

    void animateMove(modlib::Vec2i delta)
    {
        const modlib::Vec2f oldPosition = pixelPosition(m_ctl->rogue()->getPosition() - delta);
        m_anim->play(m_object, oldPosition, rogue_body::kObjectLayer, moveAnimation(rogueDirFromDelta(delta)));
    }

    void animateAttack(Rogue::Damage targetId)
    {
        const modlib::Vec2i delta = attackDelta(targetId);
        m_anim->play(m_object, currentPixelPosition(), rogue_body::kObjectLayer, attackAnimation(rogueDirFromDelta(delta)));
    }

    void animateHitFlash()
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, rogue_assets::Idle[dirIndex(m_ctl->rogue()->dir())].id, rogue_body::kZ);
        animation->addStep<anim::SetWhiteStep>(m_bodySlot, true);
        animation->addStep<anim::Step>(0.08f, 0.08f);
        animation->addStep<anim::SetWhiteStep>(m_bodySlot, false);
        animation->finishBuild();

        m_anim->play(m_object, currentPixelPosition(), rogue_body::kObjectLayer, animation->id());
    }

    anim::AnimationID buildIdleAnimation(const modlib::SpriteAsset &asset)
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, asset.id, rogue_body::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID buildMoveAnimation(
        const modlib::SpriteAsset &walk,
        const modlib::SpriteAsset &idle,
        RogueDir dir)
    {
        const modlib::Vec2i delta = dirDelta(dir);
        const modlib::Vec2f to(delta.x * kRogueTilePixels, delta.y * kRogueTilePixels);

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, walk.id, rogue_body::kZ);
        animation->addStep<anim::PosStep>(
            rogue_body::kMoveSeconds,
            rogue_body::kMoveSeconds,
            m_bodySlot,
            to,
            anim::easing::easeInOutQuart
        );
        animation->addStep<anim::SetAssetStep>(m_bodySlot, idle.id, rogue_body::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID buildAttackAnimation(
        const modlib::SpriteAsset &hit,
        const modlib::SpriteAsset &idle,
        RogueDir dir)
    {
        const modlib::Vec2i delta = dirDelta(dir);
        const modlib::Vec2f sliceOffset(
            delta.x * kRogueTilePixels + kRogueTilePixels * 0.5f,
            delta.y * kRogueTilePixels + kRogueTilePixels * 0.5f
        );

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, hit.id, rogue_body::kZ);
        animation->addStep<anim::Step>(rogue_body::kAttackPoseSeconds, rogue_body::kAttackPoseSeconds);
        animation->addStep<anim::SetPosStep>(m_sliceSlot, sliceOffset);
        animation->addStep<anim::SetRotationStep>(m_sliceSlot, sliceRotationDeg(dir));

        for (const auto &slice : rogue_assets::Slice) {
            animation->addStep<anim::SetAssetStep>(m_sliceSlot, slice.id, rogue_slice::kZ);
            animation->addStep<anim::Step>(rogue_slice::kFrameSeconds, rogue_slice::kFrameSeconds);
        }

        animation->addStep<anim::DelSpriteStep>(m_sliceSlot);
        animation->addStep<anim::SetAssetStep>(m_bodySlot, idle.id, rogue_body::kZ);
        animation->finishBuild();

        return animation->id();
    }

    anim::AnimationID idleAnimation(RogueDir dir) const
    {
        return m_anims.idle[dirIndex(dir)];
    }

    anim::AnimationID moveAnimation(RogueDir dir) const
    {
        return m_anims.move[dirIndex(dir)];
    }

    anim::AnimationID attackAnimation(RogueDir dir) const
    {
        return m_anims.attack[dirIndex(dir)];
    }

    static int dirIndex(RogueDir dir)
    {
        return static_cast<int>(dir);
    }

    static modlib::Vec2i dirDelta(RogueDir dir)
    {
        switch (dir) {
        case RogueDir::up:
            return {0, -1};
        case RogueDir::down:
            return {0, 1};
        case RogueDir::left:
            return {-1, 0};
        case RogueDir::right:
            return {1, 0};
        }

        return {0, 1};
    }

    static float sliceRotationDeg(RogueDir dir)
    {
        switch (dir) {
        case RogueDir::right:
            return 0.0f;
        case RogueDir::down:
            return 90.0f;
        case RogueDir::left:
            return 180.0f;
        case RogueDir::up:
            return -90.0f;
        }

        return 0.0f;
    }

    modlib::Vec2i attackDelta(Rogue::Damage targetId) const
    {
        if (auto *target = m_ctl->map()->getEntity(static_cast<modlib::Entity::ID>(targetId))) {
            const modlib::Vec2i raw = target->getPosition() - m_ctl->rogue()->getPosition();
            if (std::abs(raw.x) + std::abs(raw.y) == 1) {
                return raw;
            }
        }

        return dirDelta(m_ctl->rogue()->dir());
    }

    modlib::Vec2f currentPixelPosition() const
    {
        return pixelPosition(m_ctl->rogue()->getPosition());
    }

    static modlib::Vec2f pixelPosition(modlib::Vec2i cell)
    {
        return modlib::Vec2f(cell.x * kRogueTilePixels, cell.y * kRogueTilePixels);
    }
};
