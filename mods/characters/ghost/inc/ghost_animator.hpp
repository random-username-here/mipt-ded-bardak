#pragma once

#include "Animator.hpp"
#include "AssetManager.hpp"
#include "Map.hpp"
#include "Vec2.hpp"
#include "ghost_controller.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>

constexpr float kGhostTilePixels = 16.0f;

namespace gh_body {

struct Config {
	static constexpr Rectf kClip = {0, 0, 14, 14};
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
Asset(std::string_view id, const std::string &file, Rectf clip = TConfig::kClip)
{
	return {
	    .id = id,
	    .file = file,
	    .clip = clip,
	    .size = TConfig::kSize,
	};
}

struct Frame {
	modlib::SpriteAsset sprite;
	float seconds = 0.0f;
};

const std::string BlinkyTileset = ASSETS_DIR "/units/ghost/blinky/tileset.png";
const std::string FrightenedTileset = ASSETS_DIR "/units/ghost/frightened/tileset.png";

const auto Idle = Asset<gh_body::Config>("gh.idle", BlinkyTileset, {0, 0, 14, 14});

const std::array<Frame, 2> DownFrames = {{
	{Asset<gh_body::Config>("gh.dn0", BlinkyTileset, {0, 0, 14, 14}),  0.20f},
	{Asset<gh_body::Config>("gh.dn1", BlinkyTileset, {14, 0, 14, 14}), 0.08f},
}};

const std::array<Frame, 2> LeftFrames = {{
	{Asset<gh_body::Config>("gh.lt0", BlinkyTileset, {0, 14, 14, 14}),  0.20f},
	{Asset<gh_body::Config>("gh.lt1", BlinkyTileset, {14, 14, 14, 14}), 0.20f},
}};

const std::array<Frame, 2> RightFrames = {{
	{Asset<gh_body::Config>("gh.rt0", BlinkyTileset, {0, 28, 14, 14}),  0.20f},
	{Asset<gh_body::Config>("gh.rt1", BlinkyTileset, {14, 28, 14, 14}), 0.20f},
}};

const std::array<Frame, 2> UpFrames = {{
	{Asset<gh_body::Config>("gh.up0", BlinkyTileset, {0, 42, 14, 14}),  0.20f},
	{Asset<gh_body::Config>("gh.up1", BlinkyTileset, {14, 42, 14, 14}), 0.20f},
}};

const std::array<Frame, 8> DeathFrames = {{
	{Asset<gh_body::Config>("gh.fr0", FrightenedTileset, {0, 0, 14, 14}),   0.20f},
	{Asset<gh_body::Config>("gh.fr1", FrightenedTileset, {14, 0, 14, 14}),  0.20f},
	{Asset<gh_body::Config>("gh.fr2", FrightenedTileset, {0, 14, 14, 14}),  0.20f},
	{Asset<gh_body::Config>("gh.fr3", FrightenedTileset, {14, 14, 14, 14}), 0.20f},
	{Asset<gh_body::Config>("gh.fr4", FrightenedTileset, {0, 28, 14, 14}),  0.20f},
	{Asset<gh_body::Config>("gh.fr5", FrightenedTileset, {14, 28, 14, 14}), 0.20f},
	{Asset<gh_body::Config>("gh.fr6", FrightenedTileset, {0, 42, 14, 14}),  0.20f},
	{Asset<gh_body::Config>("gh.fr7", FrightenedTileset, {14, 42, 14, 14}), 0.20f},
}};

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
		registerFrames(gh_assets::DownFrames);
		registerFrames(gh_assets::LeftFrames);
		registerFrames(gh_assets::RightFrames);
		registerFrames(gh_assets::UpFrames);
		registerFrames(gh_assets::DeathFrames);
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
		m_anims.move_d = buildMoveAnimation(gh_assets::DownFrames, {0, 1});
		m_anims.move_u = buildMoveAnimation(gh_assets::UpFrames, {0, -1});
		m_anims.move_l = buildMoveAnimation(gh_assets::LeftFrames, {-1, 0});
		m_anims.move_r = buildMoveAnimation(gh_assets::RightFrames, {1, 0});
	}

	template <std::size_t N>
	void registerFrames(const std::array<gh_assets::Frame, N> &frames)
	{
		for (const auto &frame : frames) {
			m_assets->registerSprite(frame.sprite);
		}
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
		addFrame(*animation, gh_assets::DeathFrames[4]);
		addFrame(*animation, gh_assets::DeathFrames[5]);
		addFrame(*animation, gh_assets::DeathFrames[4]);
		addFrame(*animation, gh_assets::DeathFrames[5]);
		addFrame(*animation, gh_assets::DeathFrames[0]);
		addFrame(*animation, gh_assets::DeathFrames[1]);
		addFrame(*animation, gh_assets::DeathFrames[2]);
		addFrame(*animation, gh_assets::DeathFrames[3]);
		addFrame(*animation, gh_assets::DeathFrames[6]);
		addFrame(*animation, gh_assets::DeathFrames[7]);
		animation->finishBuild();
		return animation->id();
	}

	template <std::size_t N>
	anim::AnimationID buildMoveAnimation(
	    const std::array<gh_assets::Frame, N> &frames,
	    modlib::Vec2i delta)
	{
		const modlib::Vec2f to(delta.x * kGhostTilePixels, delta.y * kGhostTilePixels);

		auto *animation = m_anim->newAnimation();
		animation->addStep<anim::PosStep>(
		    gh_body::kMoveSeconds,
		    0.0f,
		    m_body_slot,
		    to,
		    anim::easing::easeInOutQuart);
		addLoopedFrameSequence(*animation, frames, gh_body::kMoveSeconds);
		animation->addStep<anim::SetAssetStep>(m_body_slot, gh_assets::Idle.id, gh_body::kZ);
		animation->finishBuild();
		return animation->id();
	}

	void addFrame(anim::Animation &animation, const gh_assets::Frame frame)
	{
		animation.addStep<anim::SetAssetStep>(m_body_slot, frame.sprite.id, gh_body::kZ);
		animation.addStep<anim::Step>(frame.seconds, frame.seconds);
	}

	template <std::size_t N>
	void addFrameSequence(anim::Animation &animation, const std::array<gh_assets::Frame, N> &frames)
	{
		for (const auto &frame : frames) {
			addFrame(animation, frame);
		}
	}

	template <std::size_t N>
	void addLoopedFrameSequence(
	    anim::Animation &animation,
	    const std::array<gh_assets::Frame, N> &frames,
	    float minSeconds)
	{
		float elapsed = 0.0f;
		std::size_t frameIndex = 0;
		while (elapsed < minSeconds) {
			const auto &frame = frames[frameIndex % frames.size()];
			animation.addStep<anim::SetAssetStep>(m_body_slot, frame.sprite.id, gh_body::kZ);
			animation.addStep<anim::Step>(frame.seconds, frame.seconds);
			elapsed += frame.seconds;
			++frameIndex;
		}
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
