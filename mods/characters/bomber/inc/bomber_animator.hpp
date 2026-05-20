#pragma once

#include "Animator.hpp"
#include "AssetManager.hpp"
#include "Map.hpp"
#include "Vec2.hpp"
#include "bomber_controller.hpp"

#include <array>
#include <cmath>
#include <string>
#include <string_view>

constexpr float kBomberTilePixels = 16.0f;

static const std::string kBomberAssetFile = ASSETS_DIR "/units/bomber/all.png";

namespace bomber_body {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 16, 16};
    static constexpr modlib::Vec2f kSize = {kBomberTilePixels * 16.0f / 20.0f, kBomberTilePixels};
};

constexpr int kZ = 0;
constexpr int kObjectLayer = 2;
static constexpr float kMoveSeconds   = 0.16f;

} // namespace bomber_body

namespace bomber_spawn {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 32, 32};
    static constexpr modlib::Vec2f kSize = {kBomberTilePixels * 2.0f, kBomberTilePixels * 2.0f};
};

constexpr int kZ = 4;
static constexpr float kFrameSeconds = 0.045f;

} // namespace bomber_spawn

namespace bomber_dead {

constexpr int kZ = -4;

} // namespace bomber_dead

namespace bomber_assets {

static const modlib::SpriteAsset Idle[4] = {
    {.id = "b.id.d", .file =  kBomberAssetFile,.clip = {8,   12, 16, 20}, .size = bomber_body::Config::kSize},
    {.id = "b.id.u", .file =  kBomberAssetFile,.clip = {80,  12, 16, 20}, .size = bomber_body::Config::kSize},
    {.id = "b.id.l", .file =  kBomberAssetFile,.clip = {176, 12, -16, 20}, .size = bomber_body::Config::kSize},
    {.id = "b.id.r", .file =  kBomberAssetFile,.clip = {152, 12, 16, 20}, .size = bomber_body::Config::kSize}
};

static const modlib::SpriteAsset Walk[4] = {
    {.id = "b.w.d", .file = kBomberAssetFile, .clip ={32,  12, 16, 20},  .size = bomber_body::Config::kSize},
    {.id = "b.w.u", .file = kBomberAssetFile, .clip ={104, 12, 16, 20},  .size = bomber_body::Config::kSize},
    {.id = "b.w.l", .file = kBomberAssetFile, .clip ={224, 12, -16, 20}, .size = bomber_body::Config::kSize},
    {.id = "b.w.r", .file = kBomberAssetFile, .clip ={200, 12, 16, 20},  .size = bomber_body::Config::kSize},
};

inline modlib::SpriteAsset spawnSprite(std::string_view id, int col)
{
    return {
        .id     = id,
        .file   = ASSETS_DIR "/units/archer/anim_spawn.png",
        .clip   = {static_cast<float>(col * 32), 0, 32, 32},
        .size   = bomber_spawn::Config::kSize,
        .origin = {8, 8},
    };
}

static const modlib::SpriteAsset Dead = {
    .id   = "b.dead",
    .file = kBomberAssetFile,
    .clip = {224, 328, 12, 12},
    .size = {12.0f, 12.0f},
};

static const std::array<modlib::SpriteAsset, 6> Spawn = {
    spawnSprite("b.sp.1", 0),
    spawnSprite("b.sp.2", 1),
    spawnSprite("b.sp.3", 2),
    spawnSprite("b.sp.4", 3),
    spawnSprite("b.sp.5", 4),
    spawnSprite("b.sp.6", 5),
};

} // namespace bomber_assets

class BomberAnimator {
    BomberCtl              *m_ctl    = nullptr;
    anim::AnimationManager *m_anim   = nullptr;
    modlib::AssetManager   *m_assets = nullptr;
    anim::AnimatedObjectID  m_object      = anim::NO_ANIMATION_OBJECT;
    anim::AnimatedObjectID  m_flashObject = anim::NO_ANIMATION_OBJECT;
    anim::AnimatedObjectID  m_spawnObject = anim::NO_ANIMATION_OBJECT;
    anim::SpriteSlotID      m_spawnSlot = 0;
    anim::SpriteSlotID      m_flashSlot = 0;
    anim::SpriteSlotID      m_bodySlot  = 0;

    struct {
        std::array<anim::AnimationID, 4> idle{};
        std::array<anim::AnimationID, 4> move{};
        std::array<anim::AnimationID, 4> shoot{};
    } m_anims;

public:
    BomberAnimator(BomberCtl *ctl, anim::AnimationManager *anim, modlib::AssetManager *assets)
        : m_ctl(ctl)
        , m_anim(anim)
        , m_assets(assets)
    {
        m_object      = m_anim->newObject();
        m_flashObject = m_anim->newObject();
        m_spawnObject = m_anim->newObject();

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
        m_assets->registerSprite(bomber_assets::Dead);
        for (const auto &asset : bomber_assets::Spawn) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : bomber_assets::Idle) {
            m_assets->registerSprite(asset);
        }
        for (const auto &asset : bomber_assets::Walk) {
            m_assets->registerSprite(asset);
        }
    }

    void buildAnimations()
    {
        for (int i = 0; i < 4; ++i) {
            const auto dir = static_cast<BomberDir>(i);
            m_anims.idle [i] = buildIdleAnimation (bomber_assets::Idle  [i]);
            m_anims.move [i] = buildMoveAnimation (bomber_assets::Walk  [i], bomber_assets::Idle[i], dir);
        }
    }

    void subscribeOnEvents()
    {
        m_ctl->bomber()->EvEntityMoved.subscribe([this](modlib::Vec2i delta) {
            animateMove(delta);
        });
        m_ctl->bomber()->EvDamaged.subscribe([this](EC::Stats::Health::HP) {
            animateHitFlash();
        });
        m_ctl->bomber()->EvDeath.subscribe([this]() {
            animateDeath();
        });
    }

    void animateSpawn()
    {
        auto *animation = m_anim->newAnimation();

        for (const auto &spawn : bomber_assets::Spawn) {
            animation->addStep<anim::SetAssetStep>(m_spawnSlot, spawn.id, bomber_spawn::kZ);
            animation->addStep<anim::Step>(bomber_spawn::kFrameSeconds, bomber_spawn::kFrameSeconds);
        }

        animation->addStep<anim::DelSpriteStep>(m_spawnSlot);
        animation->finishBuild();

        m_anim->play(
            m_spawnObject,
            currentPixelPosition(),
            bomber_body::kObjectLayer + 2,
            animation->id()
        );
    }

    void animateDeath()
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, bomber_assets::Dead.id, bomber_dead::kZ);
        animation->finishBuild();

        m_anim->play(
            m_object,
            currentPixelPosition(),
            bomber_body::kObjectLayer - 1,
            animation->id()
        );
    }

    void animateIdle()
    {
        m_anim->play(m_object, currentPixelPosition(), bomber_body::kObjectLayer, idleAnimation(m_ctl->bomber()->dir()));
    }

    void animateMove(modlib::Vec2i delta)
    {
        if (m_ctl->bomber()->getCurrentHP() <= 0) {
            return;
        }
        const modlib::Vec2f oldPosition = pixelPosition(m_ctl->bomber()->getPosition() - delta);
        m_anim->play(m_object, oldPosition, bomber_body::kObjectLayer, moveAnimation(bomberDirFromDelta(delta)));
    }

    void animateHitFlash()
    {
        if (m_ctl->bomber()->getCurrentHP() <= 0) {
            return;
        }
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(
            m_flashSlot,
            bomber_assets::Idle[dirIndex(m_ctl->bomber()->dir())].id,
            bomber_body::kZ
        );
        animation->addStep<anim::SetWhiteStep>(m_flashSlot, true);
        animation->addStep<anim::Step>(0.08f, 0.08f);
        animation->addStep<anim::DelSpriteStep>(m_flashSlot);
        animation->finishBuild();

        m_anim->play(
            m_flashObject,
            currentPixelPosition(),
            bomber_body::kObjectLayer + 1,
            animation->id()
        );
    }

    anim::AnimationID buildIdleAnimation(const modlib::SpriteAsset &asset)
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, asset.id, bomber_body::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID buildMoveAnimation(
        const modlib::SpriteAsset &walk,
        const modlib::SpriteAsset &idle,
        BomberDir dir)
    {
        const modlib::Vec2i delta = dirDelta(dir);
        const modlib::Vec2f to(delta.x * kBomberTilePixels, delta.y * kBomberTilePixels);

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, walk.id, bomber_body::kZ);
        animation->addStep<anim::PosStep>(
            bomber_body::kMoveSeconds,
            bomber_body::kMoveSeconds,
            m_bodySlot,
            to,
            anim::easing::easeInOutQuart
        );
        animation->addStep<anim::SetAssetStep>(m_bodySlot, idle.id, bomber_body::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID idleAnimation(BomberDir dir) const
    {
        return m_anims.idle[dirIndex(dir)];
    }

    anim::AnimationID moveAnimation(BomberDir dir) const
    {
        return m_anims.move[dirIndex(dir)];
    }

    static int dirIndex(BomberDir dir)
    {
        return static_cast<int>(dir);
    }

    static modlib::Vec2i dirDelta(BomberDir dir)
    {
        switch (dir) {
        case BomberDir::up:
            return {0, -1};
        case BomberDir::down:
            return {0, 1};
        case BomberDir::left:
            return {-1, 0};
        case BomberDir::right:
            return {1, 0};
        }

        return {0, 1};
    }

    modlib::Vec2i attackDelta(Bomber::Damage targetId) const
    {
        if (auto *target = m_ctl->map()->getEntity(static_cast<modlib::Entity::ID>(targetId))) {
            const modlib::Vec2i raw = target->getPosition() - m_ctl->bomber()->getPosition();
            if (combat_grid::inArcherRange(m_ctl->bomber()->getPosition(), target->getPosition())) {
                return raw;
            }
        }

        return dirDelta(m_ctl->bomber()->dir());
    }

    modlib::Vec2f currentPixelPosition() const
    {
        return pixelPosition(m_ctl->bomber()->getPosition());
    }

    static modlib::Vec2f pixelPosition(modlib::Vec2i cell)
    {
        return modlib::Vec2f(cell.x * kBomberTilePixels, cell.y * kBomberTilePixels);
    }
};
