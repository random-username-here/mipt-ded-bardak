/**
 * \file
 * \brief Animation system
 * \author Didyk Ivan
 * \date 2026-05-06
 */
#pragma once
#include "AssetManager.hpp"
#include "Event.hpp"
#include "Vec2.hpp"
#include "modlib_mod.hpp"
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace anim {

using modlib::Vec2f;
using modlib::Vec2i;

using SpriteSlotID = size_t;
using AnimatedObjectID = size_t;
using AnimationID = size_t;

using EasingFunction = std::function<float(float)>;
static const AnimatedObjectID NO_ANIMATION_OBJECT = (size_t) -1;
static const AnimationID NO_ANIMATION = (size_t) -1;

namespace easing {

static inline float linear(float t) { return t; }

static inline float easeInOutQuart(float x) {
	return x < 0.5 ? 8 * x * x * x * x : 1 - std::pow(-2 * x + 2, 4) / 2;
}

}

struct Sprite {
	modlib::SpriteID asset_id;
    Vec2f pos{};
    Vec2f lastPos{};
    float rotation = 0;
    float lastRotation = 0;
	int z = 0;
};

using SpriteBySlot = std::unordered_map<SpriteSlotID, Sprite>;

/**
 * \brief One unit of animation script
 *
 * Step has two times: time this step takes to execute and delay
 * before starting next step. Multiple states can be ran in parallel.
 * Conflicting steps are UB.
 *
 * This thing can be used as delay step, it does nothing.
 */
struct Step {
    float stepTime, delayTime;

    template<typename T>
    T *as() { return dynamic_cast<T*>(this); }
    template<typename T>
    const T *as() const { return dynamic_cast<const T*>(this); }
    template<typename T>
    bool is() const { return as<T>() != nullptr; }

    Step() : Step(0,0 ) {}
    Step(float st, float dt) :stepTime(st), delayTime(dt) {}

    virtual ~Step() {}
};

/** Set sprite's texture, creates sprite if it didn't exist. Done before wait. */
class SetAssetStep : public Step {
    SpriteSlotID slot;
	modlib::SpriteID asset_id;
    int z;

  public:
    SetAssetStep(SpriteSlotID s, modlib::SpriteID a, int spriteZ = 0)
        : slot(s), asset_id(a), z(spriteZ) {}
	
	void apply(SpriteBySlot& sprites) const {
        auto &sprite = sprites[slot];
		sprite.asset_id = asset_id;
        sprite.z = z;
	}
};

/** Delete sprite */
class DelSpriteStep : public Step {
    SpriteSlotID slot;

  public:
    DelSpriteStep(SpriteSlotID s) : slot(s) {}

	void apply(SpriteBySlot& sprites) const {
		sprites.erase(slot);
	}
};

/** Call some method */
class CallbackStep : public Step {
    std::function<void()> callback;
	
  public:
    CallbackStep(std::function<void()> cb) : callback(cb) {}

	void apply(SpriteBySlot& /* sprites */) const {
		callback();
	}

	void end(SpriteBySlot& /* sprites */) const {
		callback();
	}
};

inline float lerp(float a, float b, float fac) {
	return a * (1 - fac) + b * fac;
}

/** Move sprite from one position to another, using specified easing function. */
class PosStep : public Step {
    SpriteSlotID slot;
	Vec2f to;
	EasingFunction easing;

  public:
    PosStep(float st, float dt, SpriteSlotID s, Vec2f t, EasingFunction ef = easing::linear)
        : Step(st, dt), slot(s), to(t), easing(ef) {}

	void apply(SpriteBySlot& sprites, float frac) const {
		auto &spr = sprites[slot];
		spr.pos.x = lerp(spr.lastPos.x, to.x, easing(frac));
		spr.pos.y = lerp(spr.lastPos.y, to.y, easing(frac));
	}

	void end(SpriteBySlot& sprites, bool interrupt) const {
		auto &spr = sprites[slot];
		if (!interrupt) spr.pos = to;
		spr.lastPos = spr.pos; 
	}
};

/** Rotate sprite */
class RotationStep : public Step {
    SpriteSlotID slot;
	float angle;
	EasingFunction easing;

  public:
    RotationStep(float st, float dt, SpriteSlotID s, float a, EasingFunction ef = easing::linear)
        : Step(st, dt), slot(s), angle(a), easing(ef) {}

	void apply(SpriteBySlot& sprites, float frac) const {
		auto &spr = sprites[slot];
		spr.rotation = lerp(spr.lastRotation, angle, easing(frac));
	}
		
	void end(SpriteBySlot& sprites, bool interrupt) const {
		auto &spr = sprites[slot];
		if (!interrupt) spr.rotation = angle;
		spr.lastRotation = spr.rotation;
	}
};

class AnimationManager;

/** Animation -- a vector of steps */
class Animation {
    friend class AnimationManager;
	
    std::vector<std::unique_ptr<Step>> m_steps;
    AnimationID m_id;
    AnimationManager *m_manager;

    Animation(AnimationID id, AnimationManager *mgr) : m_id(id), m_manager(mgr) {}

public:
    template<typename T, typename ...Args>
    T* addStep(Args ...args) {
        auto v = std::make_unique<T>(args...);
        auto ptr = v.get();
        m_steps.push_back(std::move(v));
        return ptr;
    }

    void finishBuild();

    AnimationID id() const { return m_id; }
    AnimationManager *manager() const { return m_manager; }
    auto &steps() const { return m_steps; }

    // TODO: static Animation loadFromAsset(id); 
};

/** 
 * \brief Manager for registering, playing and attaching play handlers.
 */
class AnimationManager : public Mod {
    friend class Animation;

    Event<AnimatedObjectID, Vec2f, int, AnimationID> m_onAnim;
    Event<const Animation*> m_onReg;

protected:
    virtual void animationBuilt(Animation *an) = 0;

    Animation constructAnimation(size_t id) {
        return Animation(id, this);
    }

public:
    virtual AnimatedObjectID newObject() = 0;
    virtual SpriteSlotID newSpriteSlot() = 0;

    virtual Animation* newAnimation() = 0;
    virtual const Animation* animationFixUp(AnimationID id) const = 0;

    /** Play given animation on object. If another animation is playing, interrupt it. */
    virtual void play(AnimatedObjectID obj, Vec2f off, int layer, AnimationID id) = 0;

    auto &onPlay() { return m_onAnim; }
    auto &onRegister() { return m_onReg; }
};

inline void Animation::finishBuild() { manager()->animationBuilt(this); }

};
