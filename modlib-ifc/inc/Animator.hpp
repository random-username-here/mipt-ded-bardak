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

static const AnimationID NO_ANIMATION = (size_t) -1;

class Bezier;

/**
 * \brief Compiled bezier easing function 
 * This has a vector of computed time-value points for easy lerping.
 */
class CompiledBezier {
    friend class Bezier;
    std::vector<Vec2f> pts;
    CompiledBezier() = default;
public:
    float at(float t) const;
};

/** 
 * \brief Bezier function defenition 
 * You can generate those on site like https://www.cssportal.com/css-cubic-bezier-generator/
 * c1x & c2x must be ∈ [0, 1]
 */
class Bezier {
    Vec2f m_cp1, m_cp2;
public:
    Bezier(float c1x, float c1y, float c2x, float c2y)
        :m_cp1(c1x, c1y), m_cp2(c2x, c2y) {}

    Vec2f cp1() const { return m_cp1; }
    Vec2f cp2() const { return m_cp2; }

    CompiledBezier compile(size_t numPts = 100) const;

    // Couple of pre-defined curves
    static Bezier linear() { return Bezier(0, 0, 1, 1); }
    static Bezier ease() { return Bezier(0.25, 0.1, 0.25, 1); }
};

/**
 * \brief One unit of animation script
 *
 * Step has two times: time this step takes to execute and delay
 * before starting next step. Multiple states can be ran in parallel.
 * Conflicting steps are UB.
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
    SpriteID sprite; Vec2f to; /*Bezier easing;*/
    PosStep(float st, float dt, SpriteID s, Vec2f t) :Step(st, dt), sprite(s), to(t) {}
};

/** Rotate sprite */
struct RotationStep : public Step {
    SpriteID sprite; float angle; /* Bezier easing;*/
    RotationStep(float st, float dt, SpriteID s, float a) :Step(st, dt), sprite(s), angle(a) {}
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
