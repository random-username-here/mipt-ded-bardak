#pragma once

#include "Animator.hpp"
#include "AssetManager.hpp"
#include "Map.hpp"
#include "Vec2.hpp"
#include "ghost_controller.hpp"

#include <cmath>
#include <string>
#include <string_view>
#include <unordered_map>

constexpr float kGhostTilePixels = 16.0f;

namespace gh_body {

struct Config {
	static constexpr Rectf kClip = {0, 0, 16, 16};
	static constexpr Vec2f kSize = {kGhostTilePixels, kGhostTilePixels};
};

constexpr int kZ = 0;
constexpr int kObjectLayer = 0;
static constexpr float kMoveSeconds = 0.75f;

} // namespace gh_body

namespace gh_slash {

struct Config {
	static constexpr Rectf kClip = {0, 0, 32, 32};
	static constexpr Vec2f kSize = {kGhostTilePixels * 1.2f, kGhostTilePixels * 1.2f};
};

constexpr int kZ = 1;
static constexpr float kAttackFrameSeconds = 0.05f;

} // namespace gh_slash

namespace gh_assets {

template <typename TConfig>
static const modlib::SpriteAsset
Asset(std::string_view id, const std::string &file)
{
	return {
	    .id = id,
	    .file = file,
	    .clip = TConfig::kClip,
	    .size = TConfig::kSize,
	};
}

const auto Idle = Asset<gh_body::Config>("gh.idle", ASSETS_DIR "/units/ghost/down.png");
const auto Death = Asset<gh_body::Config>("gh.death", ASSETS_DIR "/units/ghost/dead.png");
const auto RunDown = Asset<gh_body::Config>("gh.run.d", ASSETS_DIR "/units/ghost/down.png");
const auto RunUp = Asset<gh_body::Config>("gh.run.u", ASSETS_DIR "/units/ghost/up.png");
const auto RunLeft = Asset<gh_body::Config>("gh.run.l", ASSETS_DIR "/units/ghost/left.png");
const auto RunRight = Asset<gh_body::Config>("gh.run.r", ASSETS_DIR "/units/ghost/right.png");
const auto Slash1 = Asset<gh_slash::Config>("gh.slh.1", ASSETS_DIR "/units/ghost/slash_01.png");
const auto Slash2 = Asset<gh_slash::Config>("gh.slh.2", ASSETS_DIR "/units/ghost/slash_02.png");
const auto Slash3 = Asset<gh_slash::Config>("gh.slh.3", ASSETS_DIR "/units/ghost/slash_03.png");
const auto Slash4 = Asset<gh_slash::Config>("gh.slh.4", ASSETS_DIR "/units/ghost/slash_04.png");

} // namespace gh_assets

class GhostAnimator {
	GhostCtl *m_ctl = nullptr;
	anim::AnimationManager *m_anim = nullptr;
	modlib::AssetManager *m_assets = nullptr;
	anim::AnimatedObjectID m_object_id = anim::NO_ANIMATION_OBJECT;
	anim::SpriteSlotID m_body_slot = 0;
	anim::SpriteSlotID m_slash_slot = 0;

	struct {
		anim::AnimationID idle = anim::NO_ANIMATION;
		anim::AnimationID death = anim::NO_ANIMATION;
		anim::AnimationID move_d = anim::NO_ANIMATION;
		anim::AnimationID move_u = anim::NO_ANIMATION;
		anim::AnimationID move_l = anim::NO_ANIMATION;
		anim::AnimationID move_r = anim::NO_ANIMATION;

		std::unordered_map<int, anim::AnimationID> attacks;
	} m_anims;

public:
	GhostAnimator(GhostCtl *ctl, anim::AnimationManager *anim, modlib::AssetManager *assets)
	    : m_ctl(ctl)
	    , m_anim(anim)
	    , m_assets(assets)
	{
		m_object_id = m_anim->newObject();
		m_body_slot = m_anim->newSpriteSlot();
		m_slash_slot = m_anim->newSpriteSlot();

		registerAssets();
		buildAnimations();
		subscribeOnEvents();
		animateIdle();
	}

private:
	void registerAssets()
	{
		m_assets->registerSprite(gh_assets::Idle);
		m_assets->registerSprite(gh_assets::Death);
		m_assets->registerSprite(gh_assets::RunDown);
		m_assets->registerSprite(gh_assets::RunLeft);
		m_assets->registerSprite(gh_assets::RunUp);
		m_assets->registerSprite(gh_assets::RunRight);
		m_assets->registerSprite(gh_assets::Slash1);
		m_assets->registerSprite(gh_assets::Slash2);
		m_assets->registerSprite(gh_assets::Slash3);
		m_assets->registerSprite(gh_assets::Slash4);
	}

	void subscribeOnEvents()
	{
		m_ctl->ghost()->EvAttack.subscribe(
		    [this](EC::Stats::Attack::Damage target_id) {
			    animateAttack(target_id);
		    });

		m_ctl->ghost()->EvEntityMoved.subscribe(
		    [this](modlib::Vec2i delta) {
			    animateMove(delta);
		    });

		m_ctl->ghost()->EvDeath.subscribe(
		    [this]() {
			    animateDeath();
		    });
	}

	void animateIdle()
	{
		m_anim->play(m_object_id, currentPixelPosition(), gh_body::kObjectLayer, m_anims.idle);
	}

	void animateMove(modlib::Vec2i delta)
	{
		const modlib::Vec2f old_position = pixelPosition(m_ctl->ghost()->getPosition() - delta);

		m_anim->play(
		    m_object_id,
		    old_position,
		    gh_body::kObjectLayer,
		    moveAnimation(delta));
	}

	void animateDeath()
	{
		m_anim->play(
		    m_object_id,
		    currentPixelPosition(),
		    gh_body::kObjectLayer - 1,
		    m_anims.death);
	}

	void animateAttack(EC::Stats::Attack::Damage target_id)
	{
		const auto tid = static_cast<modlib::Entity::ID>(target_id);
		m_anim->play(
		    m_object_id,
		    currentPixelPosition(),
		    gh_body::kObjectLayer,
		    attackAnimation(attackDelta(tid)));
	}

	void buildAnimations()
	{
		m_anims.idle = buildIdleAnimation();
		m_anims.death = buildDeathAnimation();
		m_anims.move_d = buildMoveAnimation(gh_assets::RunDown, {0, 1});
		m_anims.move_u = buildMoveAnimation(gh_assets::RunUp, {0, -1});
		m_anims.move_l = buildMoveAnimation(gh_assets::RunLeft, {-1, 0});
		m_anims.move_r = buildMoveAnimation(gh_assets::RunRight, {1, 0});
	}

	anim::AnimationID buildIdleAnimation()
	{
		auto *animation = m_anim->newAnimation();
		animation->addStep<anim::SetAssetStep>(m_body_slot, gh_assets::Idle.id, gh_body::kZ);
		animation->finishBuild();
		return animation->id();
	}

	anim::AnimationID buildDeathAnimation()
	{
		auto *animation = m_anim->newAnimation();
		animation->addStep<anim::SetAssetStep>(m_body_slot, gh_assets::Death.id, gh_body::kZ);
		animation->finishBuild();
		return animation->id();
	}

	anim::AnimationID buildMoveAnimation(const modlib::SpriteAsset &asset, modlib::Vec2i delta)
	{
		const modlib::Vec2f to(delta.x * kGhostTilePixels, delta.y * kGhostTilePixels);

		auto *animation = m_anim->newAnimation();
		animation->addStep<anim::SetAssetStep>(m_body_slot, asset.id, gh_body::kZ);
		animation->addStep<anim::PosStep>(
		    gh_body::kMoveSeconds,
		    gh_body::kMoveSeconds,
		    m_body_slot,
		    to,
		    anim::easing::easeInOutQuart);
		animation->addStep<anim::SetAssetStep>(m_body_slot, gh_assets::Idle.id, gh_body::kZ);
		animation->finishBuild();
		return animation->id();
	}

	anim::AnimationID buildAttackAnimation(modlib::Vec2i delta)
	{
		const modlib::Vec2f slash_offset(delta.x * kGhostTilePixels, delta.y * kGhostTilePixels);

		auto *animation = m_anim->newAnimation();
		animation->addStep<anim::SetAssetStep>(m_body_slot, gh_assets::Idle.id, gh_body::kZ);
		animation->addStep<anim::PosStep>(0.0f, 0.0f, m_slash_slot, slash_offset);

		animation->addStep<anim::SetAssetStep>(m_slash_slot, gh_assets::Slash1.id, gh_slash::kZ);
		animation->addStep<anim::Step>(gh_slash::kAttackFrameSeconds, gh_slash::kAttackFrameSeconds);
		animation->addStep<anim::SetAssetStep>(m_slash_slot, gh_assets::Slash2.id, gh_slash::kZ);
		animation->addStep<anim::Step>(gh_slash::kAttackFrameSeconds, gh_slash::kAttackFrameSeconds);
		animation->addStep<anim::SetAssetStep>(m_slash_slot, gh_assets::Slash3.id, gh_slash::kZ);
		animation->addStep<anim::Step>(gh_slash::kAttackFrameSeconds, gh_slash::kAttackFrameSeconds);
		animation->addStep<anim::SetAssetStep>(m_slash_slot, gh_assets::Slash4.id, gh_slash::kZ);
		animation->addStep<anim::Step>(gh_slash::kAttackFrameSeconds, gh_slash::kAttackFrameSeconds);
		animation->addStep<anim::DelSpriteStep>(m_slash_slot);
		animation->finishBuild();

		return animation->id();
	}

	anim::AnimationID moveAnimation(modlib::Vec2i delta) const
	{
		if (std::abs(delta.x) > std::abs(delta.y)) {
			return delta.x < 0 ? m_anims.move_l : m_anims.move_r;
		}

		return delta.y < 0 ? m_anims.move_u : m_anims.move_d;
	}

	anim::AnimationID attackAnimation(modlib::Vec2i delta)
	{
		const int key = directionKey(delta);
		auto it = m_anims.attacks.find(key);
		if (it != m_anims.attacks.end()) {
			return it->second;
		}

		const anim::AnimationID animation = buildAttackAnimation(delta);
		m_anims.attacks.emplace(key, animation);
		return animation;
	}

	static int directionKey(modlib::Vec2i delta)
	{
		return (delta.x + 1) * 3 + (delta.y + 1);
	}

	modlib::Vec2f currentPixelPosition() const
	{
		return pixelPosition(m_ctl->ghost()->getPosition());
	}

	static modlib::Vec2f pixelPosition(modlib::Vec2i cell)
	{
		return modlib::Vec2f(cell.x * kGhostTilePixels, cell.y * kGhostTilePixels);
	}

	modlib::Vec2i attackDelta(modlib::Entity::ID target_id) const
	{
		modlib::Vec2i delta(1, 0);

		if (auto *target = m_ctl->map()->getEntity(target_id)) {
			const modlib::Vec2i raw = target->getPosition() - m_ctl->ghost()->getPosition();
			delta.x = raw.x == 0 ? 0 : (raw.x < 0 ? -1 : 1);
			delta.y = raw.y == 0 ? 0 : (raw.y < 0 ? -1 : 1);

			if (delta.x == 0 && delta.y == 0) {
				delta.x = 1;
			}
		}

		return delta;
	}
};
