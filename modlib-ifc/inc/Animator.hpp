/**
 * \file
 * \brief Animation system
 * \author Didyk Ivan
 * \date 2026-05-06
 */
#pragma once
#include "Event.hpp"
#include "Vec2.hpp"
#include "modlib_mod.hpp"
#include <cmath>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace anim {

using modlib::Vec2f;
using modlib::Vec2i;

using ResourceID = std::string_view; // FIXME: temporary solution
using SpriteID = size_t;
using AnimationID = size_t;
using AnimatedObjectID = uintptr_t;
using EasingFunction = std::function<float(float)>;
static const AnimationID NO_ANIMATION = (size_t) -1;

namespace easing {
    static inline float linear(float t) { return t; }
    static inline float easeInOutQuart(float x) {
        return x < 0.5 ? 8 * x * x * x * x : 1 - std::pow(-2 * x + 2, 4) / 2;
    }
};

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

    Step(float st, float dt) :stepTime(st), delayTime(dt) {}

    virtual ~Step() {}
};

/** Set sprite's texture, creates sprite if it didn't exist. Done before wait. */
struct SetImageStep : public Step {
    SpriteID id; ResourceID rid;
    SetImageStep(SpriteID i, ResourceID r) :Step(0, 0), id(i), rid(r) {}
};

/** Delete sprite */
struct DelSpriteStep : public Step {
    SpriteID id;
    DelSpriteStep(SpriteID i) :Step(0, 0), id(i) {}
};

/** Move sprite from one position to another, using specified easing function. */
struct PosStep : public Step {
    SpriteID sprite; Vec2f to; EasingFunction easing;
    PosStep(float st, float dt, SpriteID s, Vec2f t, EasingFunction ef = easing::linear) 
        :Step(st, dt), sprite(s), to(t), easing(ef) {}
};

/** Rotate sprite */
struct RotationStep : public Step {
    SpriteID sprite; float angle; EasingFunction easing;
    RotationStep(float st, float dt, SpriteID s, float a, EasingFunction ef = easing::linear)
        :Step(st, dt), sprite(s), angle(a), easing(ef) {}
};

/** Call some method */
struct CallbackStep : public Step {
    std::function<void()> callback;
    CallbackStep(std::function<void()> cb) :Step(0, 0), callback(cb) {}
};

class AnimationManager;

/** Animation -- a vector of steps */
class Animation {
    friend class AnimationManager;
    std::vector<std::unique_ptr<Step>> m_steps;
    AnimationID m_id;
    AnimationManager *m_manager;

    Animation(AnimationID id, AnimationManager *mgr) :m_id(id), m_manager(mgr) {}
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

    virtual Animation* newAnimation() = 0;

    /** Play given animation on object. If another animation is playing, interrupt it. */
    virtual void play(AnimatedObjectID obj, Vec2f off, int layer, AnimationID id) = 0;

    auto &onPlay() { return m_onAnim; }
    auto &onRegister() { return m_onReg; }
};

inline void Animation::finishBuild() { manager()->animationBuilt(this); }

};
