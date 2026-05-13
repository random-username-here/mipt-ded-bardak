#pragma once

#include "Animator.hpp"
#include "AssetManager.hpp"
#include "Vec2.hpp"
#include "mage_controller.hpp"

#include <array>
#include <string>
#include <string_view>

constexpr float kMageTilePixels = 16.0f;

namespace mage_body {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 16, 16};
    static constexpr modlib::Vec2f kSize = {kMageTilePixels, kMageTilePixels};
};

constexpr int kZ = 0;
constexpr int kObjectLayer = 2;
static constexpr float kMoveSeconds     = 0.16f;
static constexpr float kCastHoldSeconds = 0.08f;

} // namespace mage_body

namespace mage_cast_fx {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 25, 24};
    static constexpr modlib::Vec2f kSize = {25, 24};
};

constexpr int kZ = 3;
static constexpr float kFrameSeconds = 0.04f;

} // namespace mage_cast_fx

namespace mage_flame {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 8, 12};
    static constexpr modlib::Vec2f kSize = {8, 12};
};

constexpr int kZ = 4;
constexpr int kObjectLayer = 3;
static constexpr float kFrameSeconds = 0.025f;

} // namespace mage_flame

namespace mage_dead {

constexpr int kZ = -4;

} // namespace mage_dead

namespace mage_assets {

inline modlib::SpriteAsset sheetSprite(std::string_view id, const std::string &file, int col)
{
    return {
        .id   = id,
        .file = file,
        .clip = {static_cast<float>(col * 16), 0, 16, 16},
        .size = mage_body::Config::kSize,
    };
}

inline modlib::SpriteAsset castFxSprite(std::string_view id, int col)
{
    return {
        .id     = id,
        .file   = ASSETS_DIR "/units/mage/anim_cast.png",
        .clip   = {static_cast<float>(col * 25), 0, 25, 24},
        .size   = mage_cast_fx::Config::kSize,
        .offset = {-4, -8},
    };
}

inline modlib::SpriteAsset flameSprite(std::string_view id, int col)
{
    return {
        .id     = id,
        .file   = ASSETS_DIR "/units/mage/anim_flame.png",
        .clip   = {static_cast<float>(col * 8), 0, 8, 12},
        .size   = mage_flame::Config::kSize,
        .offset = {4, 2},
    };
}

static const std::array<modlib::SpriteAsset, 4> Idle = {
    sheetSprite("m.id.d", ASSETS_DIR "/units/mage/mage_idle.png", 0),
    sheetSprite("m.id.u", ASSETS_DIR "/units/mage/mage_idle.png", 1),
    sheetSprite("m.id.l", ASSETS_DIR "/units/mage/mage_idle.png", 2),
    sheetSprite("m.id.r", ASSETS_DIR "/units/mage/mage_idle.png", 3),
};

static const std::array<modlib::SpriteAsset, 4> Walk = {
    sheetSprite("m.w.d", ASSETS_DIR "/units/mage/mage_walk.png", 0),
    sheetSprite("m.w.u", ASSETS_DIR "/units/mage/mage_walk.png", 1),
    sheetSprite("m.w.l", ASSETS_DIR "/units/mage/mage_walk.png", 2),
    sheetSprite("m.w.r", ASSETS_DIR "/units/mage/mage_walk.png", 3),
};

static const std::array<modlib::SpriteAsset, 4> Cast = {
    sheetSprite("m.c.d", ASSETS_DIR "/units/mage/mage_cast.png", 0),
    sheetSprite("m.c.u", ASSETS_DIR "/units/mage/mage_cast.png", 1),
    sheetSprite("m.c.l", ASSETS_DIR "/units/mage/mage_cast.png", 2),
    sheetSprite("m.c.r", ASSETS_DIR "/units/mage/mage_cast.png", 3),
};

static const modlib::SpriteAsset Dead = {
    .id   = "m.dead",
    .file = ASSETS_DIR "/units/mage/mage_dead.png",
    .clip = mage_body::Config::kClip,
    .size = mage_body::Config::kSize,
};

static const std::array<modlib::SpriteAsset, 5> CastFx = {
    castFxSprite("m.cf.1", 0),
    castFxSprite("m.cf.2", 1),
    castFxSprite("m.cf.3", 2),
    castFxSprite("m.cf.4", 3),
    castFxSprite("m.cf.5", 4),
};

static const std::array<modlib::SpriteAsset, 12> Flame = {
    flameSprite("m.fl.1",  0),
    flameSprite("m.fl.2",  1),
    flameSprite("m.fl.3",  2),
    flameSprite("m.fl.4",  3),
    flameSprite("m.fl.5",  4),
    flameSprite("m.fl.6",  5),
    flameSprite("m.fl.7",  6),
    flameSprite("m.fl.8",  7),
    flameSprite("m.fl.9",  8),
    flameSprite("m.fl.10", 9),
    flameSprite("m.fl.11", 10),
    flameSprite("m.fl.12", 11),
};

} // namespace mage_assets

class MageAnimator {
    MageCtl *m_ctl = nullptr;
    anim::AnimationManager *m_anim   = nullptr;
    modlib::AssetManager   *m_assets = nullptr;

    anim::AnimatedObjectID m_object      = anim::NO_ANIMATION_OBJECT;
    anim::AnimatedObjectID m_flashObject = anim::NO_ANIMATION_OBJECT;

    anim::SpriteSlotID m_bodySlot   = 0;
    anim::SpriteSlotID m_castFxSlot = 0;
    anim::SpriteSlotID m_flashSlot  = 0;

    struct {
        std::array<anim::AnimationID, 4> idle{};
        std::array<anim::AnimationID, 4> move{};
    } m_anims;

public:
    MageAnimator(MageCtl *ctl, anim::AnimationManager *anim, modlib::AssetManager *assets)
        : m_ctl(ctl)
        , m_anim(anim)
        , m_assets(assets)
    {
        m_object      = m_anim->newObject();
        m_flashObject = m_anim->newObject();

        m_bodySlot   = m_anim->newSpriteSlot();
        m_castFxSlot = m_anim->newSpriteSlot();
        m_flashSlot  = m_anim->newSpriteSlot();

        registerAssets();
        buildAnimations();
        subscribeOnEvents();
        animateIdle();
    }

private:
    void registerAssets()
    {
        for (const auto &asset : mage_assets::Idle) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : mage_assets::Walk) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : mage_assets::Cast) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : mage_assets::CastFx) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : mage_assets::Flame) {
            m_assets->registerSprite(asset);
        }

        m_assets->registerSprite(mage_assets::Dead);
    }

    void buildAnimations()
    {
        for (int i = 0; i < 4; ++i) {
            const auto dir = static_cast<MageDir>(i);
            m_anims.idle[i] = buildIdleAnimation(mage_assets::Idle[i]);
            m_anims.move[i] = buildMoveAnimation(mage_assets::Walk[i], mage_assets::Idle[i], dir);
        }
    }

    void subscribeOnEvents()
    {
        m_ctl->mage()->EvEntityMoved.subscribe([this](modlib::Vec2i delta) {
            animateMove(delta);
        });

        m_ctl->mage()->EvCast.subscribe([this](bmsg::Char64, modlib::Vec2i) {
            animateCast();
        });

        m_ctl->mage()->EvFlameTile.subscribe([this](modlib::Vec2i pos) {
            animateFlame(pos);
        });

        m_ctl->mage()->EvDamaged.subscribe([this](EC::Stats::Health::HP) {
            animateHitFlash();
        });

        m_ctl->mage()->EvDeath.subscribe([this]() {
            animateDeath();
        });
    }

    void animateIdle()
    {
        m_anim->play(m_object, currentPixelPosition(), mage_body::kObjectLayer, idleAnimation(m_ctl->mage()->dir()));
    }

    void animateMove(modlib::Vec2i delta)
    {
        if (m_ctl->mage()->getCurrentHP() <= 0) {
            return;
        }

        const modlib::Vec2f oldPosition = pixelPosition(m_ctl->mage()->getPosition() - delta);
        m_anim->play(m_object, oldPosition, mage_body::kObjectLayer, moveAnimation(mageDirFromDelta(delta)));
    }

    void animateCast()
    {
        if (m_ctl->mage()->getCurrentHP() <= 0) {
            return;
        }

        const MageDir dir = m_ctl->mage()->dir();
        auto *animation = m_anim->newAnimation();

        animation->addStep<anim::SetAssetStep>(m_bodySlot, mage_assets::Cast[dirIndex(dir)].id, mage_body::kZ);
        animation->addStep<anim::Step>(mage_body::kCastHoldSeconds, mage_body::kCastHoldSeconds);

        for (const auto &fx : mage_assets::CastFx) {
            animation->addStep<anim::SetAssetStep>(m_castFxSlot, fx.id, mage_cast_fx::kZ);
            animation->addStep<anim::Step>(mage_cast_fx::kFrameSeconds, mage_cast_fx::kFrameSeconds);
        }

        animation->addStep<anim::DelSpriteStep>(m_castFxSlot);
        animation->addStep<anim::SetAssetStep>(m_bodySlot, mage_assets::Idle[dirIndex(dir)].id, mage_body::kZ);
        animation->finishBuild();

        m_anim->play(m_object, currentPixelPosition(), mage_body::kObjectLayer, animation->id());
    }

    void animateFlame(modlib::Vec2i tile)
    {
        const auto object = m_anim->newObject();
        const auto slot   = m_anim->newSpriteSlot();

        auto *animation = m_anim->newAnimation();

        for (const auto &flame : mage_assets::Flame) {
            animation->addStep<anim::SetAssetStep>(slot, flame.id, mage_flame::kZ);
            animation->addStep<anim::Step>(mage_flame::kFrameSeconds, mage_flame::kFrameSeconds);
        }

        animation->addStep<anim::DelSpriteStep>(slot);
        animation->finishBuild();

        m_anim->play(object, pixelPosition(tile), mage_flame::kObjectLayer, animation->id());
    }

    void animateHitFlash()
    {
        if (m_ctl->mage()->getCurrentHP() <= 0) {
            return;
        }

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(
            m_flashSlot,
            mage_assets::Idle[dirIndex(m_ctl->mage()->dir())].id,
            mage_body::kZ
        );
        animation->addStep<anim::SetWhiteStep>(m_flashSlot, true);
        animation->addStep<anim::Step>(0.08f, 0.08f);
        animation->addStep<anim::DelSpriteStep>(m_flashSlot);
        animation->finishBuild();

        m_anim->play(
            m_flashObject,
            currentPixelPosition(),
            mage_body::kObjectLayer + 1,
            animation->id()
        );
    }

    void animateDeath()
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, mage_assets::Dead.id, mage_dead::kZ);
        animation->finishBuild();

        m_anim->play(
            m_object,
            currentPixelPosition(),
            mage_body::kObjectLayer - 1,
            animation->id()
        );
    }

    anim::AnimationID buildIdleAnimation(const modlib::SpriteAsset &asset)
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, asset.id, mage_body::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID buildMoveAnimation(
        const modlib::SpriteAsset &walk,
        const modlib::SpriteAsset &idle,
        MageDir dir)
    {
        const modlib::Vec2i delta = dirDelta(dir);
        const modlib::Vec2f to(delta.x * kMageTilePixels, delta.y * kMageTilePixels);

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, walk.id, mage_body::kZ);
        animation->addStep<anim::PosStep>(
            mage_body::kMoveSeconds,
            mage_body::kMoveSeconds,
            m_bodySlot,
            to,
            anim::easing::easeInOutQuart
        );
        animation->addStep<anim::SetAssetStep>(m_bodySlot, idle.id, mage_body::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID idleAnimation(MageDir dir) const
    {
        return m_anims.idle[dirIndex(dir)];
    }

    anim::AnimationID moveAnimation(MageDir dir) const
    {
        return m_anims.move[dirIndex(dir)];
    }

    static int dirIndex(MageDir dir)
    {
        return static_cast<int>(dir);
    }

    static modlib::Vec2i dirDelta(MageDir dir)
    {
        switch (dir) {
        case MageDir::up:
            return {0, -1};
        case MageDir::down:
            return {0, 1};
        case MageDir::left:
            return {-1, 0};
        case MageDir::right:
            return {1, 0};
        }

        return {0, 1};
    }

    modlib::Vec2f currentPixelPosition() const
    {
        return pixelPosition(m_ctl->mage()->getPosition());
    }

    static modlib::Vec2f pixelPosition(modlib::Vec2i cell)
    {
        return modlib::Vec2f(cell.x * kMageTilePixels, cell.y * kMageTilePixels);
    }
};
