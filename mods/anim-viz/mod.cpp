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

struct AnimatedObject {
    anim::SpriteBySlot sprites;
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
    std::unordered_map<TextureID, Texture2D> m_textures;
    std::mutex m_lock;

    float m_startTime = 0;

    float curTime() {
        timespec tp;
        clock_gettime(CLOCK_REALTIME, &tp);
        return (tp.tv_sec + (tp.tv_nsec / 1e9)) - m_startTime;
    }

    void windowThread() {
        InitWindow(800, 800, "Animation-based visualizer");
        SetTargetFPS(60);
		while (true) {
			BeginDrawing();
			ClearBackground(BLACK);

			drawObjects();
			
			EndDrawing();
		}
    }

	template <typename TMap, typename TCmp, typename TAct>
	void
	forEachInSorted(TMap&& map, TCmp&& cmp, TAct&& action) {
	    using TPtr = decltype(&map.begin()->second);

		std::vector<TPtr> sorted;
		sorted.reserve(map.size());
		for (auto &[_, v] : map) {
			sorted.push_back(&v);
		}

		std::sort( sorted.begin(), sorted.end(), cmp);

		for (TPtr obj : sorted) {
			action(obj);
		}
	}

	void drawObjects() {
		std::lock_guard<std::mutex> lock(m_lock);

		auto cmp = [](auto* lhs, auto* rhs) {
			return lhs->layer < rhs->layer;
		};

		auto action = [this](auto* ao) {
			processSteps(*ao);
			drawSprites(*ao);
		};

		forEachInSorted(m_objs, cmp, action);
	}

    void drawSprites(const AnimatedObject &obj) {
		auto cmp = [](auto* lhs, auto* rhs) {
			return lhs->z < rhs->z;
		};

		auto action = [this, &obj](auto* spr) {
			if (!drawTextureSprite(obj, *spr)) {
				std::cerr << "Failed to draw texture for sprite with id = " 
			              << spr->asset_id << "\n";
			}
		};

		forEachInSorted(obj.sprites, cmp, action);
    }

    bool drawTextureSprite(const AnimatedObject &obj, const anim::Sprite &s) {
        const auto sprite = m_assets->sprite(s.asset_id);
        if (!sprite) {
            return false;
        }

        Texture2D *texture = textureFor(*sprite);
        if (!texture || texture->id == 0) {
            return false;
        }

        Rectangle src = {
			.x      = sprite->clip.x,
			.y      = sprite->clip.y,
			.width  = sprite->clip.w,
			.height = sprite->clip.h,
		};

        Rectangle dst = {
			.x      = obj.pos.x + s.pos.x + sprite->offset.x,
			.y      = obj.pos.y + s.pos.y + sprite->offset.y,
			.width  = sprite->size.x,
			.height = sprite->size.y,
		};

		Vector2 origin = {
			.x = sprite->origin.x,
			.y = sprite->origin.y
		};

        DrawTexturePro(
            *texture,
            src,
            dst,
            origin,
            s.rotation,
            WHITE
        );

        return true;
    }

    Texture2D *textureFor(const modlib::SpriteAsset &sprite) {
        const uint64_t key = sprite.id.as_u64;

        auto loaded = m_textures.find(key);
        if (loaded != m_textures.end()) {
            return &loaded->second;
        }

		auto raw_tex = m_assets->bytes(sprite.id);
    	Image image = LoadImageFromMemory(
			".png",
			reinterpret_cast<const unsigned char*>(raw_tex->data()),
			raw_tex->size()
		);
    	Texture2D texture = LoadTextureFromImage(image);
		UnloadImage(image);

        if (texture.id == 0) {
            return nullptr;
        }

        SetTextureFilter(texture, TEXTURE_FILTER_POINT);

        auto inserted = m_textures.try_emplace(key, texture);
        return &inserted.first->second;
    }

	template<typename TStep, typename ...ArgsForDeduce, typename ...Args>
	bool tryStep(const anim::Step *step, void (TStep::*method)(ArgsForDeduce...) const, Args&&... args) {
		if (const auto* casted_step = step->as<TStep>()) {
			std::invoke(method, casted_step, std::forward<Args>(args)...);
			return true;
		}

		return false;
	}

    void applyStep(AnimatedObject &obj, const anim::Step *step, float frac) {
		tryStep(step, &anim::SetAssetStep::apply, obj.sprites      ) ||
		tryStep(step, &anim::DelSpriteStep::apply,obj.sprites      ) ||
		tryStep(step, &anim::CallbackStep::apply, obj.sprites      ) ||
		tryStep(step, &anim::PosStep::apply,      obj.sprites, frac) ||
		tryStep(step, &anim::RotationStep::apply, obj.sprites, frac);
    }

    void endStep(AnimatedObject &obj, const anim::Step *step, bool interrupt) {
		tryStep(step, &anim::PosStep::end,      obj.sprites, interrupt) ||
		tryStep(step, &anim::RotationStep::end, obj.sprites, interrupt) ||
		tryStep(step, &anim::CallbackStep::end, obj.sprites);
    }

    void processSteps(AnimatedObject &obj) {
        float now = curTime();
        auto anim = animationByID(obj.anim);
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

    const anim::Animation *animationByID(anim::AnimationID id) {
        auto it = m_anims.find(id);
        if (it != m_anims.end()) {
            return it->second;
        }

        const anim::Animation *animation = m_anim->animationFixUp(id);
        if (animation) {
            m_anims[id] = animation;
        }
        return animation;
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
            playAnimation(obj, off, lyr, an);
        });
    }
};

extern "C" Mod* modlib_create(ModManager*) { return new AnimatedVisualization(); }
