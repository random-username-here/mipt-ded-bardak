#pragma once

#include "Animator.hpp"
#include "AssetManager.hpp"
#include "Map.hpp"
#include "Vec2.hpp"
#include "person_controller.hpp"

#include <cmath>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

//TODO unify tile size between different persons and map
constexpr float kTilePixels = 40.0f;

namespace body {

struct Config {
	static constexpr Rectf kClip = {0, 0, 16, 16};
	static constexpr Vec2f kSize = {kTilePixels, kTilePixels};
};

constexpr int kZ = 0;
constexpr int kObjectLayer = 0;
static constexpr float kMoveSeconds = 0.18f;

}

namespace slash {

struct Config {
	static constexpr Rectf kClip = {0, 0, 32, 32};
	static constexpr Vec2f kSize = {kTilePixels * 1.2f, kTilePixels * 1.2f};
};

constexpr int kZ = 1;
static constexpr float kAttackFrameSeconds = 0.05f;

}

namespace assets {

template <typename TConfig>
static const modlib::SpriteAsset
Asset(std::string_view id, const std::string& file)
{
	return {
		.id = id,
		.file = file,
		.clip = TConfig::kClip,
		.size = TConfig::kSize,
	};
}

const auto Idle     = Asset<body::Config> ( "p.idle",  ASSETS_DIR "/units/person/rogue_down.png"      );
const auto RunDown  = Asset<body::Config> ( "p.run.d", ASSETS_DIR "/units/person/rogue_run_down.png"  );
const auto RunUp    = Asset<body::Config> ( "p.run.u", ASSETS_DIR "/units/person/rogue_run_up.png"    );
const auto RunLeft  = Asset<body::Config> ( "p.run.l", ASSETS_DIR "/units/person/rogue_run_left.png"  );
const auto RunRight = Asset<body::Config> ( "p.run.r", ASSETS_DIR "/units/person/rogue_run_right.png" );
const auto Slash1   = Asset<slash::Config>( "p.slh.1", ASSETS_DIR "/units/person/slash_01.png"   );
const auto Slash2   = Asset<slash::Config>( "p.slh.2", ASSETS_DIR "/units/person/slash_02.png"   );
const auto Slash3   = Asset<slash::Config>( "p.slh.3", ASSETS_DIR "/units/person/slash_03.png"   );
const auto Slash4   = Asset<slash::Config>( "p.slh.4", ASSETS_DIR "/units/person/slash_04.png"   );

}

class PersonAnimator {
    PersonCtl *m_ctl = nullptr;
    anim::AnimationManager *m_anim = nullptr;
    modlib::AssetManager *m_assets = nullptr;
    anim::AnimatedObjectID m_objectID = 0;
    anim::SpriteSlotID m_bodySlot = 0;
    anim::SpriteSlotID m_slashSlot = 0;

    anim::AnimationID m_idleAnimation = anim::NO_ANIMATION;
    anim::AnimationID m_moveDownAnimation = anim::NO_ANIMATION;
    anim::AnimationID m_moveUpAnimation = anim::NO_ANIMATION;
    anim::AnimationID m_moveLeftAnimation = anim::NO_ANIMATION;
    anim::AnimationID m_moveRightAnimation = anim::NO_ANIMATION;
    std::unordered_map<int, anim::AnimationID> m_attackAnimations;

public:
    PersonAnimator(PersonCtl *ctl, anim::AnimationManager *anim, modlib::AssetManager *assets)
        : m_ctl(ctl), m_anim(anim), m_assets(assets)
    {
        m_objectID = m_anim->newObject();
        m_bodySlot = m_anim->newSpriteSlot();
        m_slashSlot = m_anim->newSpriteSlot();

        registerAssets();
        buildAnimations();
		subscribeOnEvents();
        animateIdle();
    }

private:
    void registerAssets() {
        m_assets->registerSprite(assets::Idle);
        m_assets->registerSprite(assets::RunDown);
        m_assets->registerSprite(assets::RunLeft);
        m_assets->registerSprite(assets::RunUp);
        m_assets->registerSprite(assets::RunRight);
        m_assets->registerSprite(assets::Slash1);
        m_assets->registerSprite(assets::Slash2);
        m_assets->registerSprite(assets::Slash3);
        m_assets->registerSprite(assets::Slash4);
    }

	void subscribeOnEvents()
	{
        m_ctl->person()->EvAttack.subscribe(
            [this](Entity::ID target_id) {
                animateAttack(target_id);
            }
        );

        m_ctl->person()->EvEntityMoved.subscribe(
            [this](modlib::Vec2i delta) {
                animateMove(delta);
            }
        );
	}

    void animateIdle() {
        m_anim->play(m_objectID, currentPixelPosition(), body::kObjectLayer, m_idleAnimation);
    }

    void animateMove(modlib::Vec2i delta) {
        const modlib::Vec2f old_position = pixelPosition(m_ctl->person()->getPosition() - delta);

        m_anim->play(
            m_objectID,
            old_position,
            body::kObjectLayer,
            moveAnimation(delta)
        );
    }

    void animateAttack(Entity::ID target_id) {
        m_anim->play(
            m_objectID,
            currentPixelPosition(),
            body::kObjectLayer,
            attackAnimation(attackDelta(target_id))
        );
    }

    void buildAnimations() {
        m_idleAnimation = buildIdleAnimation();
        m_moveDownAnimation = buildMoveAnimation(assets::RunDown, modlib::Vec2i(0, 1));
        m_moveUpAnimation = buildMoveAnimation(assets::RunUp, modlib::Vec2i(0, -1));
        m_moveLeftAnimation = buildMoveAnimation(assets::RunLeft, modlib::Vec2i(-1, 0));
        m_moveRightAnimation = buildMoveAnimation(assets::RunRight, modlib::Vec2i(1, 0));
    }

    anim::AnimationID buildIdleAnimation() {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, assets::Idle.id, body::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID buildMoveAnimation(const modlib::SpriteAsset &asset, modlib::Vec2i delta) {
        const modlib::Vec2f to(delta.x * kTilePixels, delta.y * kTilePixels);

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, asset.id, body::kZ);
        animation->addStep<anim::PosStep>(
            body::kMoveSeconds,
            body::kMoveSeconds,
            m_bodySlot,
            to,
            anim::easing::easeInOutQuart
        );
        animation->addStep<anim::SetAssetStep>(m_bodySlot, assets::Idle.id, body::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID buildAttackAnimation(modlib::Vec2i delta) {
        const modlib::Vec2f slash_offset(delta.x * kTilePixels, delta.y * kTilePixels);

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_bodySlot, assets::Idle.id, body::kZ);
        animation->addStep<anim::PosStep>(0.0f, 0.0f, m_slashSlot, slash_offset);

        animation->addStep<anim::SetAssetStep>(m_slashSlot, assets::Slash1.id, slash::kZ);
        animation->addStep<anim::Step>(slash::kAttackFrameSeconds, slash::kAttackFrameSeconds);
        animation->addStep<anim::SetAssetStep>(m_slashSlot, assets::Slash2.id, slash::kZ);
        animation->addStep<anim::Step>(slash::kAttackFrameSeconds, slash::kAttackFrameSeconds);
        animation->addStep<anim::SetAssetStep>(m_slashSlot, assets::Slash3.id, slash::kZ);
        animation->addStep<anim::Step>(slash::kAttackFrameSeconds, slash::kAttackFrameSeconds);
        animation->addStep<anim::SetAssetStep>(m_slashSlot, assets::Slash4.id, slash::kZ);
        animation->addStep<anim::Step>(slash::kAttackFrameSeconds, slash::kAttackFrameSeconds);
        animation->addStep<anim::DelSpriteStep>(m_slashSlot);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID moveAnimation(modlib::Vec2i delta) const {
        if (std::abs(delta.x) > std::abs(delta.y)) {
            return delta.x < 0 ? m_moveLeftAnimation : m_moveRightAnimation;
        }

        return delta.y < 0 ? m_moveUpAnimation : m_moveDownAnimation;
    }

    anim::AnimationID attackAnimation(modlib::Vec2i delta) {
        const int key = directionKey(delta);
        auto it = m_attackAnimations.find(key);
        if (it != m_attackAnimations.end()) {
            return it->second;
        }

        const anim::AnimationID animation = buildAttackAnimation(delta);
        m_attackAnimations.emplace(key, animation);
        return animation;
    }

    static int directionKey(modlib::Vec2i delta) {
        return (delta.x + 1) * 3 + (delta.y + 1);
    }

    modlib::Vec2f currentPixelPosition() const {
        return pixelPosition(m_ctl->person()->getPosition());
    }

    static modlib::Vec2f pixelPosition(modlib::Vec2i cell) {
        return modlib::Vec2f( cell.x * kTilePixels, cell.y * kTilePixels );
    }

    modlib::Vec2i attackDelta(Person::Damage targetID) const {
        modlib::Vec2i delta(1, 0);

        if (auto *target = m_ctl->map()->getEntity(targetID)) {
            const modlib::Vec2i raw = target->getPosition() - m_ctl->person()->getPosition();
            delta.x = raw.x == 0 ? 0 : (raw.x < 0 ? -1 : 1);
            delta.y = raw.y == 0 ? 0 : (raw.y < 0 ? -1 : 1);

            if (delta.x == 0 && delta.y == 0) {
                delta.x = 1;
            }
        }

        return delta;
    }
};
