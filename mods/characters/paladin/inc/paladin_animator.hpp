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

constexpr float kPriestTilePixels = 16.0f;

namespace paladin_body {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 16, 16};
    static constexpr modlib::Vec2f kSize = {kPriestTilePixels, kPriestTilePixels};
};

constexpr int kZ = 0;
constexpr int kObjectLayer = 2;
static constexpr float kMoveSeconds       = 0.16f;
static constexpr float kAttackPoseSeconds = 0.08f;
static constexpr float kCastHoldSeconds   = 0.08f;

} // namespace paladin_body

namespace paladin_slash {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 32, 32};
    static constexpr modlib::Vec2f kSize = {kPriestTilePixels * 2.0f, kPriestTilePixels * 2.0f};
};

constexpr int kZ = 1;
static constexpr float kFrameSeconds = 0.04f;

} // namespace paladin_slash

namespace paladin_cast_fx {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 25, 24};
    static constexpr modlib::Vec2f kSize = {25, 24};
};

constexpr int kZ = 3;
static constexpr float kFrameSeconds = 0.04f;

} // namespace paladin_cast_fx

namespace paladin_lightning {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 20, 28};
    static constexpr modlib::Vec2f kSize = {20, 28};
};

constexpr int kZ = 4;
constexpr int kObjectLayer = 3;
static constexpr float kFrameSeconds = 0.035f;

} // namespace paladin_lightning

namespace paladin_shield {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 24, 26};
    static constexpr modlib::Vec2f kSize = {24, 26};
};

constexpr int kZ = 4;
static constexpr float kFrameSeconds = 0.045f;

} // namespace paladin_shield

namespace paladin_spawn {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 32, 32};
    static constexpr modlib::Vec2f kSize = {kPriestTilePixels * 2.0f, kPriestTilePixels * 2.0f};
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

inline modlib::SpriteAsset slashSprite(std::string_view id, int col)
{
    return {
        .id = id,
        .file = ASSETS_DIR "/units/paladin/anim_slicex.png",
        .clip = {static_cast<float>(col * 32), 0, 32, 32},
        .size = paladin_slash::Config::kSize,
        .origin = {16, 16},
    };
}

inline modlib::SpriteAsset castFxSprite(std::string_view id, int col)
{
    return {
        .id = id,
        .file = ASSETS_DIR "/units/paladin/anim_cast.png",
        .clip = {static_cast<float>(col * 25), 0, 25, 24},
        .size = paladin_cast_fx::Config::kSize,
        .offset = {-4, -8},
    };
}

inline modlib::SpriteAsset lightningSprite(std::string_view id, int col)
{
    return {
        .id = id,
        .file = ASSETS_DIR "/units/paladin/anim_lightning.png",
        .clip = {static_cast<float>(col * 20), 0, 20, 28},
        .size = paladin_lightning::Config::kSize,
        .offset = {-2, -12},
    };
}

inline modlib::SpriteAsset shieldSprite(std::string_view id, int col)
{
    return {
        .id = id,
        .file = ASSETS_DIR "/units/paladin/anim_shield.png",
        .clip = {static_cast<float>(col * 24), 0, 24, 26},
        .size = paladin_shield::Config::kSize,
        .offset = {-4, -10},
    };
}

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

static const std::array<modlib::SpriteAsset, 4> Idle = {
    sheetSprite("p.id.d", ASSETS_DIR "/units/paladin/priest_idle.png", 0),
    sheetSprite("p.id.u", ASSETS_DIR "/units/paladin/priest_idle.png", 1),
    sheetSprite("p.id.l", ASSETS_DIR "/units/paladin/priest_idle.png", 2),
    sheetSprite("p.id.r", ASSETS_DIR "/units/paladin/priest_idle.png", 3),
};

static const std::array<modlib::SpriteAsset, 4> Walk = {
    sheetSprite("p.w.d", ASSETS_DIR "/units/paladin/priest_walk.png", 0),
    sheetSprite("p.w.u", ASSETS_DIR "/units/paladin/priest_walk.png", 1),
    sheetSprite("p.w.l", ASSETS_DIR "/units/paladin/priest_walk.png", 2),
    sheetSprite("p.w.r", ASSETS_DIR "/units/paladin/priest_walk.png", 3),
};

static const std::array<modlib::SpriteAsset, 4> Hit = {
    sheetSprite("p.h.d", ASSETS_DIR "/units/paladin/priest_hit.png", 0),
    sheetSprite("p.h.u", ASSETS_DIR "/units/paladin/priest_hit.png", 1),
    sheetSprite("p.h.l", ASSETS_DIR "/units/paladin/priest_hit.png", 2),
    sheetSprite("p.h.r", ASSETS_DIR "/units/paladin/priest_hit.png", 3),
};

static const std::array<modlib::SpriteAsset, 4> Cast = {
    sheetSprite("p.c.d", ASSETS_DIR "/units/paladin/priest_cast.png", 0),
    sheetSprite("p.c.u", ASSETS_DIR "/units/paladin/priest_cast.png", 1),
    sheetSprite("p.c.l", ASSETS_DIR "/units/paladin/priest_cast.png", 2),
    sheetSprite("p.c.r", ASSETS_DIR "/units/paladin/priest_cast.png", 3),
};

static const modlib::SpriteAsset Dead = {
    .id   = "p.dead",
    .file = ASSETS_DIR "/units/paladin/priest_dead.png",
    .clip = paladin_body::Config::kClip,
    .size = paladin_body::Config::kSize,
};

static const std::array<modlib::SpriteAsset, 4> Slash = {
    slashSprite("p.sl.1", 0),
    slashSprite("p.sl.2", 1),
    slashSprite("p.sl.3", 2),
    slashSprite("p.sl.4", 3),
};

static const std::array<modlib::SpriteAsset, 5> CastFx = {
    castFxSprite("p.cf.1", 0),
    castFxSprite("p.cf.2", 1),
    castFxSprite("p.cf.3", 2),
    castFxSprite("p.cf.4", 3),
    castFxSprite("p.cf.5", 4),
};

static const std::array<modlib::SpriteAsset, 8> Lightning = {
    lightningSprite("p.li.1", 0),
    lightningSprite("p.li.2", 1),
    lightningSprite("p.li.3", 2),
    lightningSprite("p.li.4", 3),
    lightningSprite("p.li.5", 4),
    lightningSprite("p.li.6", 5),
    lightningSprite("p.li.7", 6),
    lightningSprite("p.li.8", 7),
};

static const std::array<modlib::SpriteAsset, 6> Shield = {
    shieldSprite("p.sh.1", 0),
    shieldSprite("p.sh.2", 1),
    shieldSprite("p.sh.3", 2),
    shieldSprite("p.sh.4", 3),
    shieldSprite("p.sh.5", 4),
    shieldSprite("p.sh.6", 5),
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

class PriestAnimator {
    PriestCtrl             *m_ctl    = nullptr;
    anim::AnimationManager *m_anim   = nullptr;
    modlib::AssetManager   *m_assets = nullptr;

    anim::AnimatedObjectID m_object      = anim::NO_ANIMATION_OBJECT;
    anim::AnimatedObjectID m_flashObject = anim::NO_ANIMATION_OBJECT;
    anim::AnimatedObjectID m_spawnObject = anim::NO_ANIMATION_OBJECT;

    anim::SpriteSlotID m_bodySlot   = 0;
    anim::SpriteSlotID m_slashSlot  = 0;
    anim::SpriteSlotID m_castFxSlot = 0;
    anim::SpriteSlotID m_shieldSlot = 0;
    anim::SpriteSlotID m_flashSlot  = 0;
    anim::SpriteSlotID m_spawnSlot  = 0;

    struct {
        std::array<anim::AnimationID, 4> idle{};
        std::array<anim::AnimationID, 4> move{};
        std::array<anim::AnimationID, 4> attack{};
    } m_anims;

public:
    PriestAnimator(PriestCtrl *ctl, anim::AnimationManager *anim, modlib::AssetManager *assets)
        : m_ctl(ctl), m_anim(anim), m_assets(assets)
    {
        m_object      = m_anim->newObject();
        m_flashObject = m_anim->newObject();
        m_spawnObject = m_anim->newObject();

        m_bodySlot   = m_anim->newSpriteSlot();
        m_slashSlot  = m_anim->newSpriteSlot();
        m_castFxSlot = m_anim->newSpriteSlot();
        m_shieldSlot = m_anim->newSpriteSlot();
        m_flashSlot  = m_anim->newSpriteSlot();
        m_spawnSlot  = m_anim->newSpriteSlot();

        registerAssets();
        buildAnimations();
        subscribeOnEvents();
        animateIdle();
        animateSpawn();
    }

private:
    void registerAssets()
    {
        for (const auto &asset : paladin_assets::Idle) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : paladin_assets::Walk) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : paladin_assets::Hit) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : paladin_assets::Cast) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : paladin_assets::Slash) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : paladin_assets::CastFx) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : paladin_assets::Lightning) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : paladin_assets::Shield) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : paladin_assets::Spawn) {
            m_assets->registerSprite(asset);
        }

        m_assets->registerSprite(paladin_assets::Dead);
    }

    void buildAnimations()
    {
        for (int i = 0; i < 4; ++i) {
            const auto dirrection = static_cast<Dir>(i);
            m_anims.idle[i]   = buildIdleAnimation  (paladin_assets::Idle[i]);
            m_anims.move[i]   = buildMoveAnimation  (paladin_assets::Walk[i], paladin_assets::Idle[i], dirrection);
            m_anims.attack[i] = buildAttackAnimation(paladin_assets::Hit[i],  paladin_assets::Idle[i], dirrection);
        }
    }

    void subscribeOnEvents()
    {
        m_ctl->paladin()->EvEntityMoved.subscribe([this](modlib::Vec2i delta) {
            animateMove(delta);
        });

        m_ctl->paladin()->EvAttack.subscribe([this](Priest::Damage targetId) {
            animateAttack(targetId);
        });

        m_ctl->paladin()->EvCast.subscribe([this](bmsg::Char64 spell, modlib::Vec2i pos) {
            animateCast(spell, pos);
        });

        m_ctl->paladin()->EvLightningTile.subscribe([this](modlib::Vec2i pos) {
            animateLightning(pos);
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

    void animateIdle()
    {
        m_anim->play(m_object, currentPixelPosition(), paladin_body::kObjectLayer, idleAnimation(m_ctl->paladin()->dirrection()));
    }

    void animateMove(modlib::Vec2i delta)
    {
        if (m_ctl->paladin()->getCurrentHP() <= 0) {
            return;
        }
        const modlib::Vec2f oldPosition = pixelPosition(m_ctl->paladin()->getPosition() - delta);
        m_anim->play(m_object, oldPosition, paladin_body::kObjectLayer, moveAnimation(Delta2Dir(delta)));
    }

    void animateAttack(Priest::Damage targetId)
    {
        if (m_ctl->paladin()->getCurrentHP() <= 0) {
            return;
        }
        const modlib::Vec2i delta = attackDelta(targetId);
        m_anim->play(m_object, currentPixelPosition(), paladin_body::kObjectLayer, attackAnimation(Delta2Dir(delta)));
    }

    void animateCast(bmsg::Char64 spell, modlib::Vec2i pos)
    {
        if (m_ctl->paladin()->getCurrentHP() <= 0) {
            return;
        }

        if (spell == "heal") {
            animateHealCast();
            return;
        }

        if (spell == "shield") {
            animateShield();
            return;
        }

        if (spell == "smite") {
            animateCastPose();
            (void)pos;
            return;
        }
    }

    void animateHealCast()
    {
        const Dir dir = m_ctl->paladin()->dirrection();
        auto *animation = m_anim->newAnimation();

        animation->addStep<anim::SetAssetStep>(m_bodySlot, paladin_assets::Cast[dirrectionIndex(dir)].id, paladin_body::kZ);
        animation->addStep<anim::Step>(paladin_body::kCastHoldSeconds, paladin_body::kCastHoldSeconds);

        for (const auto &fx : paladin_assets::CastFx) {
            animation->addStep<anim::SetAssetStep>(m_castFxSlot, fx.id, paladin_cast_fx::kZ);
            animation->addStep<anim::Step>(paladin_cast_fx::kFrameSeconds, paladin_cast_fx::kFrameSeconds);
        }

        animation->addStep<anim::DelSpriteStep>(m_castFxSlot);
        animation->addStep<anim::SetAssetStep>(m_bodySlot, paladin_assets::Idle[dirrectionIndex(dir)].id, paladin_body::kZ);
        animation->finishBuild();

        m_anim->play(m_object, currentPixelPosition(), paladin_body::kObjectLayer, animation->id());
    }

    void animateShield()
    {
        const Dir dir = m_ctl->paladin()->dirrection();
        auto *animation = m_anim->newAnimation();

        animation->addStep<anim::SetAssetStep>(m_bodySlot, paladin_assets::Cast[dirrectionIndex(dir)].id, paladin_body::kZ);
        animation->addStep<anim::Step>(paladin_body::kCastHoldSeconds, paladin_body::kCastHoldSeconds);

        for (const auto &shield : paladin_assets::Shield) {
            animation->addStep<anim::SetAssetStep>(m_shieldSlot, shield.id, paladin_shield::kZ);
            animation->addStep<anim::Step>(paladin_shield::kFrameSeconds, paladin_shield::kFrameSeconds);
        }

        animation->addStep<anim::DelSpriteStep>(m_shieldSlot);
        animation->addStep<anim::SetAssetStep>(m_bodySlot, paladin_assets::Idle[dirrectionIndex(dir)].id, paladin_body::kZ);
        animation->finishBuild();

        m_anim->play(m_object, currentPixelPosition(), paladin_body::kObjectLayer, animation->id());
    }

    void animateCastPose()
    {
        const Dir dir = m_ctl->paladin()->dirrection();
        auto *animation = m_anim->newAnimation();

        animation->addStep<anim::SetAssetStep>(m_bodySlot, paladin_assets::Cast[dirrectionIndex(dir)].id, paladin_body::kZ);
        animation->addStep<anim::Step>(paladin_body::kCastHoldSeconds, paladin_body::kCastHoldSeconds);
        animation->addStep<anim::SetAssetStep>(m_bodySlot, paladin_assets::Idle[dirrectionIndex(dir)].id, paladin_body::kZ);
        animation->finishBuild();

        m_anim->play(m_object, currentPixelPosition(), paladin_body::kObjectLayer, animation->id());
    }

    void animateLightning(modlib::Vec2i tile)
    {
        const auto object = m_anim->newObject();
        const auto slot   = m_anim->newSpriteSlot();

        auto *animation = m_anim->newAnimation();

        for (const auto &lightning : paladin_assets::Lightning) {
            animation->addStep<anim::SetAssetStep>(slot, lightning.id, paladin_lightning::kZ);
            animation->addStep<anim::Step>(paladin_lightning::kFrameSeconds, paladin_lightning::kFrameSeconds);
        }

        animation->addStep<anim::DelSpriteStep>(slot);
        animation->finishBuild();

        m_anim->play(object, pixelPosition(tile), paladin_lightning::kObjectLayer, animation->id());
    }

    void animateHitFlash()
    {
        if (m_ctl->paladin()->getCurrentHP() <= 0) {
            return;
        }
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(
            m_flashSlot,
            paladin_assets::Idle[dirrectionIndex(m_ctl->paladin()->dirrection())].id,
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
        Dir dirrection)
    {
        const modlib::Vec2i delta = dirrectionDelta(dirrection);
        const modlib::Vec2f to(delta.x * kPriestTilePixels, delta.y * kPriestTilePixels);

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
        Dir dirrection)
    {
        const modlib::Vec2i delta = dirrectionDelta(dirrection);
        const modlib::Vec2f slashOffset(
            delta.x * kPriestTilePixels + kPriestTilePixels * 0.5f,
            delta.y * kPriestTilePixels + kPriestTilePixels * 0.5f
        );

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, hit.id, paladin_body::kZ);
        animation->addStep<anim::Step>(paladin_body::kAttackPoseSeconds, paladin_body::kAttackPoseSeconds);
        animation->addStep<anim::SetPosStep>(m_slashSlot, slashOffset);
        animation->addStep<anim::SetRotationStep>(m_slashSlot, slashRotationDeg(dirrection));
        for (const auto &slash : paladin_assets::Slash) {
            animation->addStep<anim::SetAssetStep>(m_slashSlot, slash.id, paladin_slash::kZ);
            animation->addStep<anim::Step>(paladin_slash::kFrameSeconds, paladin_slash::kFrameSeconds);
        }
        animation->addStep<anim::DelSpriteStep>(m_slashSlot);
        animation->addStep<anim::SetAssetStep>(m_bodySlot, idle.id, paladin_body::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID idleAnimation(Dir dirrection) const
    {
        return m_anims.idle[dirrectionIndex(dirrection)];
    }

    anim::AnimationID moveAnimation(Dir dirrection) const
    {
        return m_anims.move[dirrectionIndex(dirrection)];
    }

    anim::AnimationID attackAnimation(Dir dirrection) const
    {
        return m_anims.attack[dirrectionIndex(dirrection)];
    }

    static int dirrectionIndex(Dir dirrection)
    {
        return static_cast<int>(dirrection);
    }

    static float slashRotationDeg(Dir dirrection)
    {
        switch (dirrection) {
        case Dir::RIGHT:
            return 0.0f;
        case Dir::DOWN:
            return 90.0f;
        case Dir::LEFT:
            return 180.0f;
        case Dir::UP:
            return -90.0f;
        }
        return 0.0f;
    }

    static modlib::Vec2i dirrectionDelta(Dir dirrection)
    {
        switch (dirrection) {
        case Dir::UP:
            return {0, -1};
        case Dir::DOWN:
            return {0, 1};
        case Dir::LEFT:
            return {-1, 0};
        case Dir::RIGHT:
            return {1, 0};
        }
        return {0, 1};
    }

    modlib::Vec2i attackDelta(Priest::Damage targetId) const
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

        return dirrectionDelta(m_ctl->paladin()->dirrection());
    }

    modlib::Vec2f currentPixelPosition() const
    {
        return pixelPosition(m_ctl->paladin()->getPosition());
    }

    static modlib::Vec2f pixelPosition(modlib::Vec2i cell)
    {
        return modlib::Vec2f(cell.x * kPriestTilePixels, cell.y * kPriestTilePixels);
    }
};
