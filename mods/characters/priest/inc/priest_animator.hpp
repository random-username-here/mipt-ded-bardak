#pragma once

#include "Animator.hpp"
#include "AssetManager.hpp"
#include "Map.hpp"
#include "Vec2.hpp"
#include "priest_controller.hpp"

#include <array>
#include <cmath>
#include <string>
#include <string_view>

constexpr float kPriestTilePixels = 16.0f;

namespace priest_body {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 16, 16};
    static constexpr modlib::Vec2f kSize = {kPriestTilePixels, kPriestTilePixels};
};

constexpr int kZ = 0;
constexpr int kObjectLayer = 2;
static constexpr float kMoveSeconds = 0.16f;
static constexpr float kAttackPoseSeconds = 0.08f;

} // namespace priest_body

namespace priest_smite {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 32, 32};
    static constexpr modlib::Vec2f kSize = {kPriestTilePixels * 2.0f, kPriestTilePixels * 2.0f};
};

constexpr int kZ = 1;
static constexpr float kFrameSeconds = 0.04f;

} // namespace priest_smite

namespace priest_spawn {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 32, 32};
    static constexpr modlib::Vec2f kSize = {kPriestTilePixels * 2.0f, kPriestTilePixels * 2.0f};
};

constexpr int kZ = 4;
static constexpr float kFrameSeconds = 0.045f;

} // namespace priest_spawn

namespace priest_dead {

constexpr int kZ = -4;

} // namespace priest_dead

namespace priest_assets {

inline modlib::SpriteAsset sheetSprite(std::string_view id, const std::string &file, int col)
{
    return {
        .id = id,
        .file = file,
        .clip = {static_cast<float>(col * 16), 0, 16, 16},
        .size = priest_body::Config::kSize,
    };
}

inline modlib::SpriteAsset smiteSprite(std::string_view id, int col)
{
    return {
        .id = id,
        .file = ASSETS_DIR "/units/priest/anim_smite.png",
        .clip = {static_cast<float>(col * 32), 0, 32, 32},
        .size = priest_smite::Config::kSize,
        .origin = {16, 16},
    };
}

static const std::array<modlib::SpriteAsset, 4> Idle = {
    sheetSprite("p.id.d", ASSETS_DIR "/units/priest/priest_idle.png", 0),
    sheetSprite("p.id.u", ASSETS_DIR "/units/priest/priest_idle.png", 1),
    sheetSprite("p.id.l", ASSETS_DIR "/units/priest/priest_idle.png", 2),
    sheetSprite("p.id.r", ASSETS_DIR "/units/priest/priest_idle.png", 3),
};

static const std::array<modlib::SpriteAsset, 4> Walk = {
    sheetSprite("p.w.d", ASSETS_DIR "/units/priest/priest_walk.png", 0),
    sheetSprite("p.w.u", ASSETS_DIR "/units/priest/priest_walk.png", 1),
    sheetSprite("p.w.l", ASSETS_DIR "/units/priest/priest_walk.png", 2),
    sheetSprite("p.w.r", ASSETS_DIR "/units/priest/priest_walk.png", 3),
};

static const std::array<modlib::SpriteAsset, 4> Hit = {
    sheetSprite("p.h.d", ASSETS_DIR "/units/priest/priest_hit.png", 0),
    sheetSprite("p.h.u", ASSETS_DIR "/units/priest/priest_hit.png", 1),
    sheetSprite("p.h.l", ASSETS_DIR "/units/priest/priest_hit.png", 2),
    sheetSprite("p.h.r", ASSETS_DIR "/units/priest/priest_hit.png", 3),
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
        .file   = ASSETS_DIR "/units/priest/anim_spawn.png",
        .clip   = {static_cast<float>(col * 32), 0, 32, 32},
        .size   = priest_spawn::Config::kSize,
        .origin = {8, 8},
    };
}

static const modlib::SpriteAsset Dead = {
    .id   = "p.dead",
    .file = ASSETS_DIR "/units/priest/priest_dead.png",
    .clip = priest_body::Config::kClip,
    .size = priest_body::Config::kSize,
};

static const std::array<modlib::SpriteAsset, 6> Spawn = {
    spawnSprite("p.sp.1", 0),
    spawnSprite("p.sp.2", 1),
    spawnSprite("p.sp.3", 2),
    spawnSprite("p.sp.4", 3),
    spawnSprite("p.sp.5", 4),
    spawnSprite("p.sp.6", 5),
};

} // namespace priest_assets

class PriestAnimator {
    PriestCtrl              *m_ctl    = nullptr;
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
    PriestAnimator(PriestCtrl *ctl, anim::AnimationManager *anim, modlib::AssetManager *assets)
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
        m_assets->registerSprite(priest_assets::Dead);
        for (const auto &asset : priest_assets::Spawn) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : priest_assets::Idle) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : priest_assets::Walk) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : priest_assets::Hit) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : priest_assets::Slash) {
            m_assets->registerSprite(asset);
        }
    }

    void buildAnimations()
    {
        for (int i = 0; i < 4; ++i) {
            const auto dirrection = static_cast<Dir>(i);
            m_anims.idle[i]   = buildIdleAnimation  (priest_assets::Idle[i]);
            m_anims.move[i]   = buildMoveAnimation  (priest_assets::Walk[i], priest_assets::Idle[i], dirrection);
            m_anims.attack[i] = buildAttackAnimation(priest_assets::Hit [i], priest_assets::Idle[i], dirrection);
        }
    }

    void subscribeOnEvents()
    {
        m_ctl->priest()->EvEntityMoved.subscribe([this](modlib::Vec2i delta) {
            animateMove(delta);
        });

        m_ctl->priest()->EvAttack.subscribe([this](Priest::Damage targetId) {
            animateAttack(targetId);
        });

        m_ctl->priest()->EvDamaged.subscribe([this](EC::Stats::Health::HP) {
            animateHitFlash();
        });

        m_ctl->priest()->EvDeath.subscribe([this]() {
            animateDeath();
        });
    }

    void animateSpawn()
    {
        auto *animation = m_anim->newAnimation();

        for (const auto &spawn : priest_assets::Spawn) {
            animation->addStep<anim::SetAssetStep>(m_spawnSlot, spawn.id, priest_spawn::kZ);
            animation->addStep<anim::Step>(priest_spawn::kFrameSeconds, priest_spawn::kFrameSeconds);
        }

        animation->addStep<anim::DelSpriteStep>(m_spawnSlot);
        animation->finishBuild();

        m_anim->play(
            m_spawnObject,
            currentPixelPosition(),
            priest_body::kObjectLayer + 2,
            animation->id()
        );
    }

    void animateDeath()
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, priest_assets::Dead.id, priest_dead::kZ);
        animation->finishBuild();

        m_anim->play(
            m_object,
            currentPixelPosition(),
            priest_body::kObjectLayer - 1,
            animation->id()
        );
    }

    void animateIdle()
    {
        m_anim->play(m_object, currentPixelPosition(), priest_body::kObjectLayer, idleAnimation(m_ctl->priest()->dirrection()));
    }

    void animateMove(modlib::Vec2i delta)
    {
        if (m_ctl->priest()->getCurrentHP() <= 0) {
            return;
        }
        const modlib::Vec2f oldPosition = pixelPosition(m_ctl->priest()->getPosition() - delta);
        m_anim->play(m_object, oldPosition, priest_body::kObjectLayer, moveAnimation(Delta2Dir(delta)));
    }

    void animateAttack(Priest::Damage targetId)
    {
        if (m_ctl->priest()->getCurrentHP() <= 0) {
            return;
        }
        const modlib::Vec2i delta = attackDelta(targetId);
        m_anim->play(m_object, currentPixelPosition(), priest_body::kObjectLayer, attackAnimation(Delta2Dir(delta)));
    }

    void animateHitFlash()
    {
        if (m_ctl->priest()->getCurrentHP() <= 0) {
            return;
        }
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(
            m_flashSlot,
            priest_assets::Idle[dirrectionIndex(m_ctl->priest()->dirrection())].id,
            priest_body::kZ
        );
        animation->addStep<anim::SetWhiteStep>(m_flashSlot, true);
        animation->addStep<anim::Step>(0.08f, 0.08f);
        animation->addStep<anim::DelSpriteStep>(m_flashSlot);
        animation->finishBuild();

        m_anim->play(
            m_flashObject,
            currentPixelPosition(),
            priest_body::kObjectLayer + 1,
            animation->id()
        );
    }

    anim::AnimationID buildIdleAnimation(const modlib::SpriteAsset &asset)
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, asset.id, priest_body::kZ);
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
        animation->addStep<anim::SetAssetStep>(m_bodySlot, walk.id, priest_body::kZ);
        animation->addStep<anim::PosStep>(
            priest_body::kMoveSeconds,
            priest_body::kMoveSeconds,
            m_bodySlot,
            to,
            anim::easing::easeInOutQuart
        );
        animation->addStep<anim::SetAssetStep>(m_bodySlot, idle.id, priest_body::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID buildAttackAnimation(
        const modlib::SpriteAsset &hit,
        const modlib::SpriteAsset &idle,
        Dir dirrection)
    {
        const modlib::Vec2i delta = dirrectionDelta(dirrection);
        const modlib::Vec2f smiteOffset(
            delta.x * kPriestTilePixels + kPriestTilePixels * 0.5f,
            delta.y * kPriestTilePixels + kPriestTilePixels * 0.5f
        );

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, hit.id, priest_body::kZ);
        animation->addStep<anim::Step>(priest_body::kAttackPoseSeconds, priest_body::kAttackPoseSeconds);
        animation->addStep<anim::SetPosStep>(m_smiteSlot, smiteOffset);
        animation->addStep<anim::SetRotationStep>(m_smiteSlot, smiteRotationDeg(dirrection));
        for (const auto &smite : priest_assets::Slash) {
            animation->addStep<anim::SetAssetStep>(m_smiteSlot, smite.id, priest_smite::kZ);
            animation->addStep<anim::Step>(priest_smite::kFrameSeconds, priest_smite::kFrameSeconds);
        }
        animation->addStep<anim::DelSpriteStep>(m_smiteSlot);
        animation->addStep<anim::SetAssetStep>(m_bodySlot, idle.id, priest_body::kZ);
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

    static float smiteRotationDeg(Dir dirrection)
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
            const modlib::Vec2i raw = target->getPosition() - m_ctl->priest()->getPosition();

            if (std::abs(raw.x) <= 1 && std::abs(raw.y) <= 1 && (std::abs(raw.x) + std::abs(raw.y)) > 0) {
                if (raw.x != 0) {
                    return {raw.x, 0};
                }

                return {0, raw.y};
            }
        }

        return dirrectionDelta(m_ctl->priest()->dirrection());
    }

    modlib::Vec2f currentPixelPosition() const
    {
        return pixelPosition(m_ctl->priest()->getPosition());
    }

    static modlib::Vec2f pixelPosition(modlib::Vec2i cell)
    {
        return modlib::Vec2f(cell.x * kPriestTilePixels, cell.y * kPriestTilePixels);
    }
};
