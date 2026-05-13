#pragma once

#include "Animator.hpp"
#include "AssetManager.hpp"
#include "Vec2.hpp"
#include "pacman_controller.hpp"

#include <cmath>
#include <string>
#include <string_view>

constexpr float kPacmanTilePixels = 40.0f;

namespace pm_body {

struct Config {
	static constexpr Rectf kClip = {0, 0, 14, 14};
	static constexpr Vec2f kSize = {kPacmanTilePixels, kPacmanTilePixels};
};

constexpr int kZ = 0;
constexpr int kObjectLayer = 0;
static constexpr float kMoveSeconds = 0.75f;

} // namespace pm_body

namespace pm_assets {

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

const auto Idle = Asset<pm_body::Config>("pm.idle", ASSETS_DIR "/units/pacman/down.png");
const auto Death = Asset<pm_body::Config>("pm.death", ASSETS_DIR "/units/pacman/dead.png");
const auto RunDown = Asset<pm_body::Config>("pm.run.d", ASSETS_DIR "/units/pacman/down.png");
const auto RunUp = Asset<pm_body::Config>("pm.run.u", ASSETS_DIR "/units/pacman/up.png");
const auto RunLeft = Asset<pm_body::Config>("pm.run.l", ASSETS_DIR "/units/pacman/left.png");
const auto RunRight = Asset<pm_body::Config>("pm.run.r", ASSETS_DIR "/units/pacman/right.png");

} // namespace pm_assets

class PacmanAnimator {
	PacmanCtl *m_ctl = nullptr;
	anim::AnimationManager *m_anim = nullptr;
	modlib::AssetManager *m_assets = nullptr;
	anim::AnimatedObjectID m_object_id = anim::NO_ANIMATION_OBJECT;
	anim::SpriteSlotID m_body_slot = 0;

	struct {
		anim::AnimationID idle = anim::NO_ANIMATION;
		anim::AnimationID death = anim::NO_ANIMATION;
		anim::AnimationID move_d = anim::NO_ANIMATION;
		anim::AnimationID move_u = anim::NO_ANIMATION;
		anim::AnimationID move_l = anim::NO_ANIMATION;
		anim::AnimationID move_r = anim::NO_ANIMATION;
	} m_anims;

public:
	PacmanAnimator(PacmanCtl *ctl, anim::AnimationManager *anim, modlib::AssetManager *assets)
	    : m_ctl(ctl)
	    , m_anim(anim)
	    , m_assets(assets)
	{
		m_object_id = m_anim->newObject();
		m_body_slot = m_anim->newSpriteSlot();

		registerAssets();
		buildAnimations();
		subscribeOnEvents();
		animateIdle();
	}

private:
	void registerAssets()
	{
		m_assets->registerSprite(pm_assets::Idle);
		m_assets->registerSprite(pm_assets::Death);
		m_assets->registerSprite(pm_assets::RunDown);
		m_assets->registerSprite(pm_assets::RunLeft);
		m_assets->registerSprite(pm_assets::RunUp);
		m_assets->registerSprite(pm_assets::RunRight);
	}

	void subscribeOnEvents()
	{
		m_ctl->pacman()->EvEntityMoved.subscribe(
		    [this](modlib::Vec2i delta) {
			    animateMove(delta);
		    });

		m_ctl->pacman()->EvDeath.subscribe(
		    [this]() {
			    animateDeath();
		    });
	}

	void animateIdle()
	{
		m_anim->play(m_object_id, currentPixelPosition(), pm_body::kObjectLayer, m_anims.idle);
	}

	void animateMove(modlib::Vec2i delta)
	{
		const modlib::Vec2f old_position = pixelPosition(m_ctl->pacman()->getPosition() - delta);

		m_anim->play(
		    m_object_id,
		    old_position,
		    pm_body::kObjectLayer,
		    moveAnimation(delta));
	}

	void animateDeath()
	{
		m_anim->play(
		    m_object_id,
		    currentPixelPosition(),
		    pm_body::kObjectLayer - 1,
		    m_anims.death);
	}

	void buildAnimations()
	{
		m_anims.idle = buildIdleAnimation();
		m_anims.death = buildDeathAnimation();
		m_anims.move_d = buildMoveAnimation(pm_assets::RunDown, {0, 1});
		m_anims.move_u = buildMoveAnimation(pm_assets::RunUp, {0, -1});
		m_anims.move_l = buildMoveAnimation(pm_assets::RunLeft, {-1, 0});
		m_anims.move_r = buildMoveAnimation(pm_assets::RunRight, {1, 0});
	}

	anim::AnimationID buildIdleAnimation()
	{
		auto *animation = m_anim->newAnimation();
		animation->addStep<anim::SetAssetStep>(m_body_slot, pm_assets::Idle.id, pm_body::kZ);
		animation->finishBuild();
		return animation->id();
	}

	anim::AnimationID buildDeathAnimation()
	{
		auto *animation = m_anim->newAnimation();
		animation->addStep<anim::SetAssetStep>(m_body_slot, pm_assets::Death.id, pm_body::kZ);
		animation->finishBuild();
		return animation->id();
	}

	anim::AnimationID buildMoveAnimation(const modlib::SpriteAsset &asset, modlib::Vec2i delta)
	{
		const modlib::Vec2f to(delta.x * kPacmanTilePixels, delta.y * kPacmanTilePixels);

		auto *animation = m_anim->newAnimation();
		animation->addStep<anim::SetAssetStep>(m_body_slot, asset.id, pm_body::kZ);
		animation->addStep<anim::PosStep>(
		    pm_body::kMoveSeconds,
		    pm_body::kMoveSeconds,
		    m_body_slot,
		    to,
		    anim::easing::easeInOutQuart);
		animation->addStep<anim::SetAssetStep>(m_body_slot, pm_assets::Idle.id, pm_body::kZ);
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

	modlib::Vec2f currentPixelPosition() const
	{
		return pixelPosition(m_ctl->pacman()->getPosition());
	}

	static modlib::Vec2f pixelPosition(modlib::Vec2i cell)
	{
		return modlib::Vec2f(cell.x * kPacmanTilePixels, cell.y * kPacmanTilePixels);
	}
};
