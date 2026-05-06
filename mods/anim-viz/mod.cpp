#include "Animator.hpp"
#include "BmServerModule.hpp"
#include "Map.hpp"
#include "Timer.hpp"
#include "Vec2.hpp"
#include "modlib_manager.hpp"
#include "raylib.h"
#include <cmath>
#include <ctime>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>

using modlib::Vec2f;

using TextureID = size_t;

struct Sprite {
    std::string_view tex = ""; // FIXME: this is debug thing
    Vec2f lastPos = Vec2f(0, 0), pos = Vec2f(0, 0);
    float rotation = 0, lastRotation = 0;
};

struct AnimatedObject {
    std::unordered_map<anim::SpriteID, Sprite> sprites;
    std::unordered_map<const anim::Step*, float> runningSteps; // step -> start time
    size_t cursor;
    float cursorTime;
    anim::AnimationID anim = anim::NO_ANIMATION;
    Vec2f pos;
    int layer;
};

Vector2 v2v(modlib::Vec2f v) {
    return Vector2 { .x = v.x, .y = v.y };
}


class AnimatedVisualization : public modlib::BmServerModule {

    std::string_view id() const override { return "isd.bardak.anim-viz"; }
    std::string_view brief() const override { return "Raylib-based visualization based on animated objects"; }
    ModVersion version() const override { return ModVersion(0, 0, 1); }

    modlib::Level *m_map;
    modlib::Timer *m_timer;
    anim::AnimationManager *m_anim;
    std::thread m_winThread;
    std::unordered_map<anim::AnimatedObjectID, AnimatedObject> m_objs;
    std::unordered_map<anim::AnimationID, const anim::Animation*> m_anims;
    std::mutex m_lock;

    float m_startTime = 0;

    float curTime() {
        timespec tp;
        clock_gettime(CLOCK_REALTIME, &tp);
        return (tp.tv_sec + (tp.tv_nsec / 1e9)) - m_startTime;
    }

    void drawSprites(const AnimatedObject &obj) {
        for (auto &[id, s] : obj.sprites) {
            Vec2f pos = obj.pos + s.pos;
            Vec2f d = { std::cos(s.rotation), std::sin(s.rotation) };
            DrawLineV(v2v(pos), v2v(pos + d * 40), RED);
            DrawLineV(v2v(pos), v2v(pos + Vec2f(d.y, -d.x) * 40), GREEN);
            DrawCircleV(v2v(pos), 5, WHITE);
        }
    }

    float lerp(float a, float b, float fac) {
        return a * (1 - fac) + b * fac;
    }

    void applyStep(AnimatedObject &obj, const anim::Step *step, float frac) {
        if (auto setImage = step->as<anim::SetImageStep>()) {
            obj.sprites[setImage->id].tex = setImage->rid;
        } else if (auto delSprite = step->as<anim::DelSpriteStep>()) {
            obj.sprites.erase(delSprite->id);
        } else if (auto posStep = step->as<anim::PosStep>()) {
            auto &spr = obj.sprites[posStep->sprite];
            spr.pos.x = lerp(spr.lastPos.x, posStep->to.x, frac);
            spr.pos.y = lerp(spr.lastPos.y, posStep->to.y, frac);
        } else if (auto rotStep = step->as<anim::RotationStep>()) {
            auto &spr = obj.sprites[rotStep->sprite];
            spr.rotation = lerp(spr.lastRotation, rotStep->angle, frac);
        }
    }

    void endStep(AnimatedObject &obj, const anim::Step *step, bool interrupt) {
        if (auto posStep = step->as<anim::PosStep>()) {
            auto &spr = obj.sprites[posStep->sprite];
            if (!interrupt) spr.pos = posStep->to;
            spr.lastPos = spr.pos; 
        } else if (auto rotStep = step->as<anim::RotationStep>()) {
            auto &spr = obj.sprites[rotStep->sprite];
            if (!interrupt) spr.rotation = rotStep->angle;
            spr.lastRotation = spr.rotation;
        }
    }

    void processSteps(AnimatedObject &obj) {
        float now = curTime();
        auto anim = m_anims[obj.anim];

        std::vector<const anim::Step*> toDelete;
        for (auto [step, startTime] : obj.runningSteps) {
            if (startTime + step->stepTime < now) {
                endStep(obj, step, false);
                toDelete.push_back(step);
            }
        }

        for (auto i : toDelete)
            obj.runningSteps.erase(i);

        while (obj.cursor < anim->steps().size() && obj.cursorTime <= now) {
            auto step = anim->steps()[obj.cursor].get();
            obj.runningSteps.insert({ step, obj.cursorTime });
            obj.cursor++;
            obj.cursorTime += step->delayTime;
        }

        for (auto [step, startTime] : obj.runningSteps) {
            applyStep(obj, step, step->stepTime ? (now - startTime) / step->stepTime : 0);
        }
    }

    void windowThread() {
        InitWindow(800, 800, "Animation-based visualizer");
        SetTargetFPS(60);
        while (1) {
            BeginDrawing();
                ClearBackground(BLACK);
                {
                    std::lock_guard<std::mutex> lock(m_lock);
                    for (auto &[id, obj] : m_objs) {
                        processSteps(obj);
                        drawSprites(obj);
                    }
                }
            EndDrawing();
        }
    }

    void onResolveDeps(ModManager *mm) override {
        m_map = mm->requireAnyOfType<modlib::Level>("Visualization needs a Level");
        m_timer = mm->requireAnyOfType<modlib::Timer>("Visualization needs a Timer");
        m_anim = mm->requireAnyOfType<anim::AnimationManager>("Visualization needs Animator");
        m_startTime = curTime();
    }

    void playAnimation(anim::AnimatedObjectID obj, anim::Vec2f off, int lyr, anim::AnimationID an) {
        std::lock_guard<std::mutex> lg(m_lock);
        auto &ao = m_objs[obj];
        for (auto [i, t] : ao.runningSteps)
            endStep(ao, i, true);
        ao.runningSteps.clear();
        ao.sprites.clear();
        ao.cursor = 0;
        ao.cursorTime = curTime();
        ao.pos = off;
        ao.layer = lyr;
        ao.anim = an;
    }

    void onDepsResolved(ModManager *) override {
        m_winThread = std::thread([this](){ windowThread(); });

        m_anim->onRegister().subscribe([this](const anim::Animation *an){
            m_anims[an->id()] = an;
        });
        m_anim->onPlay().subscribe([this](anim::AnimatedObjectID obj, anim::Vec2f off, int lyr, anim::AnimationID an){
            std::cerr << "play animation " << an << "\n";
            playAnimation(obj, off, lyr, an);
        });

        // FIXME: remove this from here, this is for debugging purposes
        auto an = m_anim->newAnimation();
        an->addStep<anim::SetImageStep>(0, "img");
        an->addStep<anim::PosStep>(0, 0, 0, Vec2f(100, 100));
        an->addStep<anim::PosStep>(5, 0, 0, Vec2f(600, 600));
        an->addStep<anim::RotationStep>(5, 5, 0, M_PI * 20);
        an->addStep<anim::PosStep>(5, 5, 0, Vec2f(100, 100));
        an->finishBuild();

        m_anim->play(0, Vec2f(0, 0), 0, an->id());
    }
};

extern "C" Mod* modlib_create(ModManager*) { return new AnimatedVisualization(); }
