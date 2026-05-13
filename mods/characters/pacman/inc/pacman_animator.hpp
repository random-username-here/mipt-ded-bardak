#pragma once

#include "Animator.hpp"
#include "AssetManager.hpp"
#include "Vec2.hpp"
#include "pacman_controller.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <string_view>

constexpr float kPacmanTilePixels = 16.0f;

namespace pm_body {

struct Config {
	static constexpr Vec2f kSize = {kPacmanTilePixels, kPacmanTilePixels};
};

constexpr int kZ = 0;
constexpr int kObjectLayer = 0;
static constexpr float kMoveSeconds = 0.75f;

} // namespace pm_body

namespace pm_assets {

template <typename TConfig>
static const modlib::SpriteAsset
Asset(std::string_view id, Rectf clip)
{
    return {
        .id = id,
        .file = ASSETS_DIR "/units/pacman/tileset.png",
        .clip = clip,
        .size = TConfig::kSize,
    };
}

struct Frame {
	modlib::SpriteAsset sprite;
	float seconds = 0.0f;
};

const auto Idle = Asset<pm_body::Config>("pm.idle", {28, 0, 14, 14});

const std::array<Frame, 11> DeathFrames = {{
	{Asset<pm_body::Config>("pm.d0",  {28, 28, 14, 14}), 0.1f},
	{Asset<pm_body::Config>("pm.d1",  {0, 42, 14, 14}),  0.1f},
	{Asset<pm_body::Config>("pm.d2",  {14, 42, 14, 14}), 0.1f},
	{Asset<pm_body::Config>("pm.d3",  {28, 42, 14, 14}), 0.1f},
	{Asset<pm_body::Config>("pm.d4",  {0, 56, 14, 14}),  0.1f},
	{Asset<pm_body::Config>("pm.d5",  {14, 56, 14, 14}), 0.1f},
	{Asset<pm_body::Config>("pm.d6",  {28, 56, 14, 14}), 0.1f},
	{Asset<pm_body::Config>("pm.d7",  {0, 70, 14, 14}),  0.1f},
	{Asset<pm_body::Config>("pm.d8",  {14, 70, 14, 14}), 0.1f},
	{Asset<pm_body::Config>("pm.d9",  {28, 70, 14, 14}), 0.1f},
	{Asset<pm_body::Config>("pm.d10", {0, 84, 14, 14}),  0.3f},
}};

const std::array<Frame, 4> LeftFrames = {{
	{Asset<pm_body::Config>("pm.l0", {0, 14, 14, 14}),  0.1f},
	{Asset<pm_body::Config>("pm.l1", {14, 14, 14, 14}), 0.1f},
	{Asset<pm_body::Config>("pm.l2", {0, 14, 14, 14}),  0.1f},
	{Asset<pm_body::Config>("pm.l3", {28, 0, 14, 14}),  0.1f},
}};

const std::array<Frame, 4> UpFrames = {{
	{Asset<pm_body::Config>("pm.u0", {14, 28, 14, 14}), 0.1f},
	{Asset<pm_body::Config>("pm.u1", {28, 28, 14, 14}), 0.1f},
	{Asset<pm_body::Config>("pm.u2", {14, 28, 14, 14}), 0.1f},
	{Asset<pm_body::Config>("pm.u3", {28, 0, 14, 14}),  0.1f},
}};

const std::array<Frame, 4> RightFrames = {{
	{Asset<pm_body::Config>("pm.r0", {28, 14, 14, 14}), 0.1f},
	{Asset<pm_body::Config>("pm.r1", {0, 28, 14, 14}),  0.1f},
	{Asset<pm_body::Config>("pm.r2", {28, 14, 14, 14}), 0.1f},
	{Asset<pm_body::Config>("pm.r3", {28, 0, 14, 14}),  0.1f},
}};

const std::array<Frame, 4> DownFrames = {{
	{Asset<pm_body::Config>("pm.v0", {0, 0, 14, 14}),  0.1f},
	{Asset<pm_body::Config>("pm.v1", {14, 0, 14, 14}), 0.1f},
	{Asset<pm_body::Config>("pm.v2", {0, 0, 14, 14}),  0.1f},
	{Asset<pm_body::Config>("pm.v3", {28, 0, 14, 14}), 0.1f},
}};

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
		registerFrames(pm_assets::DeathFrames);
		registerFrames(pm_assets::LeftFrames);
		registerFrames(pm_assets::UpFrames);
		registerFrames(pm_assets::RightFrames);
		registerFrames(pm_assets::DownFrames);
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
		m_anims.move_d = buildMoveAnimation(pm_assets::DownFrames, {0, 1});
		m_anims.move_u = buildMoveAnimation(pm_assets::UpFrames, {0, -1});
		m_anims.move_l = buildMoveAnimation(pm_assets::LeftFrames, {-1, 0});
		m_anims.move_r = buildMoveAnimation(pm_assets::RightFrames, {1, 0});
	}

	template <std::size_t N>
	void registerFrames(const std::array<pm_assets::Frame, N> &frames)
	{
		for (const auto &frame : frames) {
			m_assets->registerSprite(frame.sprite);
		}
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
		addFrameSequence(*animation, pm_assets::DeathFrames);
		animation->finishBuild();
		return animation->id();
	}

	template <std::size_t N>
	anim::AnimationID buildMoveAnimation(
	    const std::array<pm_assets::Frame, N> &frames,
	    modlib::Vec2i delta)
	{
		const modlib::Vec2f to(delta.x * kPacmanTilePixels, delta.y * kPacmanTilePixels);

		auto *animation = m_anim->newAnimation();
		animation->addStep<anim::PosStep>(
		    pm_body::kMoveSeconds,
		    0.0f,
		    m_body_slot,
		    to,
		    anim::easing::easeInOutQuart);
		addLoopedFrameSequence(*animation, frames, pm_body::kMoveSeconds);
		animation->addStep<anim::SetAssetStep>(m_body_slot, pm_assets::Idle.id, pm_body::kZ);
		animation->finishBuild();
		return animation->id();
	}

	template <std::size_t N>
	void addFrameSequence(anim::Animation &animation, const std::array<pm_assets::Frame, N> &frames)
	{
		for (const auto &frame : frames) {
			animation.addStep<anim::SetAssetStep>(m_body_slot, frame.sprite.id, pm_body::kZ);
			animation.addStep<anim::Step>(frame.seconds, frame.seconds);
		}
	}

	template <std::size_t N>
	void addLoopedFrameSequence(
	    anim::Animation &animation,
	    const std::array<pm_assets::Frame, N> &frames,
	    float minSeconds)
	{
		float elapsed = 0.0f;
		std::size_t frameIndex = 0;
		while (elapsed < minSeconds) {
			const auto &frame = frames[frameIndex % frames.size()];
			animation.addStep<anim::SetAssetStep>(m_body_slot, frame.sprite.id, pm_body::kZ);
			animation.addStep<anim::Step>(frame.seconds, frame.seconds);
			elapsed += frame.seconds;
			++frameIndex;
		}
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
