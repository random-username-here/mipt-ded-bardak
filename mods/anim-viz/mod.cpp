#include "Animator.hpp"
#include "AssetManager.hpp"
#include "BmServerModule.hpp"
#include "Map.hpp"
#include "Timer.hpp"
#include "Vec2.hpp"
#include "modlib_manager.hpp"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

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
    modlib::AssetManager *m_assets;
    std::thread m_winThread;
    std::unordered_map<anim::AnimatedObjectID, AnimatedObject> m_objs;
    std::unordered_map<anim::AnimationID, const anim::Animation*> m_anims;
    std::unordered_map<uint64_t, Texture2D> m_textures;
    std::mutex m_lock;

    float m_startTime = 0;

    float curTime() {
        timespec tp;
        clock_gettime(CLOCK_REALTIME, &tp);
        return (tp.tv_sec + (tp.tv_nsec / 1e9)) - m_startTime;
    }

    void drawSprites(const AnimatedObject &obj) {
        std::vector<anim::SpriteID> spriteOrder;
        spriteOrder.reserve(obj.sprites.size());
        for (const auto &[id, _] : obj.sprites) {
            spriteOrder.push_back(id);
        }
        std::sort(spriteOrder.begin(), spriteOrder.end());

        for (anim::SpriteID id : spriteOrder) {
            const Sprite &s = obj.sprites.at(id);
            if (!drawTextureSprite(obj, s)) {
                drawDebugSprite(obj, s);
            }
        }
    }

    bool drawTextureSprite(const AnimatedObject &obj, const Sprite &s) {
        if (!m_assets || s.tex.empty()) {
            return false;
        }

        const auto sprite = m_assets->sprite(modlib::SpriteID(s.tex));
        if (!sprite) {
            return false;
        }

        Texture2D *texture = textureFor(*sprite);
        if (!texture || texture->id == 0) {
            return false;
        }

        Rectangle src;
        src.x = static_cast<float>(sprite->source.x);
        src.y = static_cast<float>(sprite->source.y);
        src.width = static_cast<float>(sprite->source.w > 0 ? sprite->source.w : texture->width);
        src.height = static_cast<float>(sprite->source.h > 0 ? sprite->source.h : texture->height);

        Rectangle dst;
        const Vec2f pos = obj.pos + s.pos;
        dst.x = pos.x + static_cast<float>(sprite->offset.x);
        dst.y = pos.y + static_cast<float>(sprite->offset.y);
        dst.width = static_cast<float>(sprite->size.x > 0 ? sprite->size.x : static_cast<int>(src.width));
        dst.height = static_cast<float>(sprite->size.y > 0 ? sprite->size.y : static_cast<int>(src.height));

        DrawTexturePro(
            *texture,
            src,
            dst,
            Vector2{
                static_cast<float>(sprite->origin.x),
                static_cast<float>(sprite->origin.y)
            },
            s.rotation,
            WHITE
        );

        return true;
    }

    void drawDebugSprite(const AnimatedObject &obj, const Sprite &s) {
        Vec2f pos = obj.pos + s.pos;
        Vec2f d = { std::cos(s.rotation), std::sin(s.rotation) };
        DrawLineV(v2v(pos), v2v(pos + d * 40), RED);
        DrawLineV(v2v(pos), v2v(pos + Vec2f(d.y, -d.x) * 40), GREEN);
        DrawCircleV(v2v(pos), 5, WHITE);
    }

    Texture2D *textureFor(const modlib::SpriteAsset &sprite) {
        const uint64_t key = sprite.id.as_u64;

        auto loaded = m_textures.find(key);
        if (loaded != m_textures.end()) {
            return &loaded->second;
        }

        Texture2D texture = LoadTexture(sprite.file.c_str());
        if (texture.id == 0) {
            return nullptr;
        }

        SetTextureFilter(texture, TEXTURE_FILTER_POINT);

        auto inserted = m_textures.emplace(key, texture);
        return &inserted.first->second;
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
            spr.pos.x = lerp(spr.lastPos.x, posStep->to.x, posStep->easing(frac));
            spr.pos.y = lerp(spr.lastPos.y, posStep->to.y, posStep->easing(frac));
        } else if (auto rotStep = step->as<anim::RotationStep>()) {
            auto &spr = obj.sprites[rotStep->sprite];
            spr.rotation = lerp(spr.lastRotation, rotStep->angle, rotStep->easing(frac));
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
        } else if (auto cb = step->as<anim::CallbackStep>()) {
            cb->callback();
        }
    }

    void processSteps(AnimatedObject &obj) {
        float now = curTime();
        auto anim = m_anims[obj.anim];
        if (!anim) return;

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
        m_assets = mm->requireAnyOfType<modlib::AssetManager>("Visualization needs AssetManager");
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
    }
};

extern "C" Mod* modlib_create(ModManager*) { return new AnimatedVisualization(); }
