#include "Animator.hpp"
#include "AssetManager.hpp"
#include "BmServerModule.hpp"
#include "Map.hpp"
#include "Timer.hpp"
#include "Vec2.hpp"
#include "modlib_manager.hpp"
#include "combat_grid.hpp"
#include "raylib.h"
#include <algorithm>
#include <ctime>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

using modlib::Vec2f;

using TextureID = size_t;

constexpr float kRenderScale       = 2.0f;
constexpr int   kLogicalTilePixels = 16;
constexpr float kHpBarFadeSeconds  = 0.5f;
constexpr int   kOverlaySplitLayer = 0;
constexpr Color kVisibilityOverlayColor = { 40, 120, 255, 55};
constexpr Color kAttackOverlayColor     = {255,  40,  40, 70};

static constexpr const char *kWhiteFlashFragShader = R"(
#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform sampler2D texture0;

void main()
{
    vec4 texel = texture(texture0, fragTexCoord);

    if (texel.a <= 0.0) {
        discard;
    }

    finalColor = vec4(1.0, 1.0, 1.0, texel.a) * fragColor;
}
)";

struct AnimatedObject {
    anim::SpriteBySlot sprites;
    std::unordered_map<const anim::Step*, float> runningSteps; // step -> start time
    size_t cursor;
    float cursorTime;
    anim::AnimationID anim = anim::NO_ANIMATION;
    Vec2f pos;
    int layer;
};

struct DebugTile {
    modlib::Vec2i pos;
};

struct HealthBarFade {
    modlib::Entity::ID entity = 0;
    modlib::Vec2i tile{};
    int hp     = 0;
    int maxHp  = 1;
    int lastHp = -1;
    float startedAt = 0.0f;
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
    Shader m_whiteFlashShader{};
    std::vector<DebugTile> m_debugVisibleTiles;
    std::vector<DebugTile> m_debugAttackTiles;
    std::vector<HealthBarFade> m_healthBarFades;
    bool m_showDebugVisibility = false;
    bool m_showDebugAttack = false;
    std::mutex m_lock;

    float m_startTime = 0;

    float curTime() {
        timespec tp;
        clock_gettime(CLOCK_REALTIME, &tp);
        return (tp.tv_sec + (tp.tv_nsec / 1e9)) - m_startTime;
    }

    void windowThread() {
        const modlib::Vec2i mapSize = m_map->getSize();

        const int windowW = std::max(
            1,
            static_cast<int>(mapSize.x * kLogicalTilePixels * kRenderScale)
        );
        const int windowH = std::max(
            1,
            static_cast<int>(mapSize.y * kLogicalTilePixels * kRenderScale)
        );

        InitWindow(windowW, windowH, "Animation-based visualizer");
        m_whiteFlashShader = LoadShaderFromMemory(nullptr, kWhiteFlashFragShader);
        SetTargetFPS(60);

        while (true) {
            if (IsKeyPressed(KEY_V)) {
                m_showDebugVisibility = !m_showDebugVisibility;
            }

            if (IsKeyPressed(KEY_A)) {
                m_showDebugAttack = !m_showDebugAttack;
            }

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
        rebuildDebugOverlays();

        std::lock_guard<std::mutex> lock(m_lock);

        std::vector<AnimatedObject*> sorted;
        sorted.reserve(m_objs.size());

        for (auto &[_, obj] : m_objs) {
            processSteps(obj);
            sorted.push_back(&obj);
        }

        std::sort(sorted.begin(), sorted.end(), [](auto *lhs, auto *rhs) {
            return lhs->layer < rhs->layer;
        });

        for (AnimatedObject *obj : sorted) {
            if (obj->layer < kOverlaySplitLayer) {
                drawSprites(*obj);
            }
        }

        drawDebugOverlays();

        for (AnimatedObject *obj : sorted) {
            if (obj->layer >= kOverlaySplitLayer) {
                drawSprites(*obj);
            }
        }

        drawHealthBars();
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
            .x      = (obj.pos.x + s.pos.x + sprite->offset.x) * kRenderScale,
            .y      = (obj.pos.y + s.pos.y + sprite->offset.y) * kRenderScale,
            .width  = sprite->size.x * kRenderScale,
            .height = sprite->size.y * kRenderScale,
        };

        Vector2 origin = {
            .x = sprite->origin.x * kRenderScale,
            .y = sprite->origin.y * kRenderScale
        };

        if (s.forceWhite && m_whiteFlashShader.id != 0) {
            BeginShaderMode(m_whiteFlashShader);
            DrawTexturePro(
                *texture,
                src,
                dst,
                origin,
                s.rotation,
                WHITE
            );
            EndShaderMode();

            return true;
        }

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
		tryStep(step, &anim::SetAssetStep::apply,    obj.sprites      ) ||
		tryStep(step, &anim::DelSpriteStep::apply,   obj.sprites      ) ||
		tryStep(step, &anim::SetPosStep::apply,      obj.sprites      ) ||
		tryStep(step, &anim::SetRotationStep::apply, obj.sprites      ) ||
		tryStep(step, &anim::SetWhiteStep::apply,    obj.sprites      ) ||
		tryStep(step, &anim::CallbackStep::apply,    obj.sprites      ) ||
		tryStep(step, &anim::PosStep::apply,         obj.sprites, frac) ||
		tryStep(step, &anim::RotationStep::apply,    obj.sprites, frac);
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

    void drawDebugOverlays()
    {
        if (m_showDebugVisibility) {
            for (const DebugTile &tile : m_debugVisibleTiles) {
                drawTileOverlay(tile.pos, kVisibilityOverlayColor);
            }
        }

        if (m_showDebugAttack) {
            for (const DebugTile &tile : m_debugAttackTiles) {
                drawTileOverlay(tile.pos, kAttackOverlayColor);
            }
        }
    }

    void drawTileOverlay(modlib::Vec2i pos, Color color)
    {
        Rectangle dst = {
            .x      = pos.x * kLogicalTilePixels * kRenderScale,
            .y      = pos.y * kLogicalTilePixels * kRenderScale,
            .width  =         kLogicalTilePixels * kRenderScale,
            .height =         kLogicalTilePixels * kRenderScale,
        };

        DrawRectangleRec(dst, color);
    }

    void drawHealthBars()
    {
        updateHealthBarFades();

        const float now = curTime();

        for (const HealthBarFade &bar : m_healthBarFades) {
            const float age = now - bar.startedAt;
            if (age < 0.0f || age >= kHpBarFadeSeconds) {
                continue;
            }

            const float alpha = 1.0f - std::max(0.0f, std::min(1.0f, age / kHpBarFadeSeconds));
            drawHpBarAt(bar.tile, bar.hp, bar.maxHp, alpha);
        }
    }

    void updateHealthBarFades()
    {
        const float now = curTime();

        for (const auto &[id, entity] : m_map->getEntityList()) {
            if (entity == nullptr || !isHealthBarUnit(entity)) {
                continue;
            }

            auto *health = dynamic_cast<EC::Stats::Health *>(entity);
            if (health == nullptr) {
                continue;
            }

            const int maxHp = static_cast<int>(health->getMaxHP());
            if (maxHp <= 0) {
                continue;
            }

            const int hp = std::max(
                0,
                std::min(maxHp, static_cast<int>(health->getCurrentHP()))
            );

            HealthBarFade *bar = findHealthBarFade(id);
            if (bar == nullptr) {
                m_healthBarFades.push_back(HealthBarFade{
                    .entity = id,
                    .tile = entity->getPosition(),
                    .hp = hp,
                    .maxHp = maxHp,
                    .lastHp = hp,
                    .startedAt = -1000000.0f,
                });
                continue;
            }

            if (hp > 0 && hp < bar->lastHp) {
                bar->tile = entity->getPosition();
                bar->hp = hp;
                bar->maxHp = maxHp;
                bar->startedAt = now;
            }

            bar->lastHp = hp;
        }

        m_healthBarFades.erase(
            std::remove_if(
                m_healthBarFades.begin(),
                m_healthBarFades.end(),
                [this, now](const HealthBarFade &bar) {
                    if (now - bar.startedAt < kHpBarFadeSeconds) {
                        return false;
                    }

                    auto *entity = m_map->getEntity(bar.entity);
                    if (entity == nullptr || !isHealthBarUnit(entity)) {
                        return true;
                    }

                    auto *health = dynamic_cast<EC::Stats::Health *>(entity);
                    if (health == nullptr) {
                        return true;
                    }

                    return health->getCurrentHP() <= 0 || health->getCurrentHP() >= health->getMaxHP();
                }
            ),
            m_healthBarFades.end()
        );
    }

    HealthBarFade *findHealthBarFade(modlib::Entity::ID id)
    {
        for (HealthBarFade &bar : m_healthBarFades) {
            if (bar.entity == id) {
                return &bar;
            }
        }

        return nullptr;
    }

    void drawHpBarAt(modlib::Vec2i pos, int hp, int maxHp, float alpha)
    {
        alpha = std::max(0.0f, std::min(1.0f, alpha));

        const float tile = kLogicalTilePixels * kRenderScale;

        const float rx = pos.x * tile;
        const float ry = pos.y * tile;

        const float frac = static_cast<float>(hp) / static_cast<float>(maxHp);

        const float outerW = tile * 0.76f;
        const float outerH = std::max(5.0f, tile * 0.13f);

        const float bx = rx + (tile - outerW) * 0.5f;
        const float by = ry - outerH - tile * 0.10f;

        const float pad = std::max(1.0f, std::floor(tile * 0.035f));
        const float innerX = bx + pad;
        const float innerY = by + pad;
        const float innerW = std::max(0.0f, outerW - pad * 2.0f);
        const float innerH = std::max(1.0f, outerH - pad * 2.0f);

        DrawRectangleV(
            Vector2{bx, by},
            Vector2{outerW, outerH},
            withAlpha(Color{18, 18, 18, 230}, alpha)
        );

        DrawRectangleV(
            Vector2{innerX, innerY},
            Vector2{innerW, innerH},
            withAlpha(Color{70, 20, 20, 230}, alpha)
        );

        DrawRectangleV(
            Vector2{innerX, innerY},
            Vector2{innerW * frac, innerH},
            withAlpha(hpColor(frac), alpha)
        );

        DrawRectangleLinesEx(
            Rectangle{bx, by, outerW, outerH},
            std::max(1.0f, std::floor(tile * 0.035f)),
            withAlpha(Color{5, 5, 5, 255}, alpha)
        );
    }

    static Color withAlpha(Color color, float alpha)
    {
        alpha = std::max(0.0f, std::min(1.0f, alpha));
        color.a = static_cast<unsigned char>(static_cast<float>(color.a) * alpha);
        return color;
    }

    static Color hpColor(float frac)
    {
        frac = std::max(0.0f, std::min(1.0f, frac));

        const unsigned char r = static_cast<unsigned char>(255.0f * (1.0f - frac));
        const unsigned char g = static_cast<unsigned char>(220.0f * frac);
        const unsigned char b = 40;

        return Color{r, g, b, 255};
    }

    static bool isHealthBarUnit(const modlib::Entity *entity)
    {
        if (entity == nullptr) {
            return false;
        }

        if (entity->getType() == modlib::Entity::BasicTypes::ROOT) {
            return false;
        }

        const auto *health = dynamic_cast<const EC::Stats::Health *>(entity);
        return health != nullptr;
    }

    void rebuildDebugOverlays()
    {
        std::vector<DebugTile> visible;
        std::vector<DebugTile> attack;

        const modlib::Vec2i size = m_map->getSize();

        for (const auto &[id, entity] : m_map->getEntityList()) {
            (void)id;

            if (entity == nullptr || !isDebugPlayer(entity)) {
                continue;
            }

            const modlib::Vec2i origin = entity->getPosition();

            for (const modlib::Vec2i off : combat_grid::visibleOffsets()) {
                const modlib::Vec2i pos{origin.x + off.x, origin.y + off.y};

                if (insideMap(pos, size)) {
                    visible.push_back({pos});
                }
            }

            for (const modlib::Vec2i off : attackOffsetsFor(entity)) {
                const modlib::Vec2i pos{origin.x + off.x, origin.y + off.y};

                if (insideMap(pos, size)) {
                    attack.push_back({pos});
                }
            }
        }

        m_debugVisibleTiles = std::move(visible);
        m_debugAttackTiles = std::move(attack);
    }

    static bool insideMap(modlib::Vec2i pos, modlib::Vec2i size)
    {
        return pos.x >= 0 && pos.y >= 0 && pos.x < size.x && pos.y < size.y;
    }

    static bool isDebugPlayer(const modlib::Entity *entity)
    {
        if (!combat_grid::isAliveHealth(entity)) {
            return false;
        }

        const std::string_view type = entity->getType();

        return type == "knight" ||
               type == "rogue" ||
               type == "archer";
    }

    static std::vector<modlib::Vec2i> attackOffsetsFor(const modlib::Entity *entity)
    {
        const std::string_view type = entity->getType();

        std::vector<modlib::Vec2i> out;

        if (type == "knight") {
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    if (dx == 0 && dy == 0) {
                        continue;
                    }

                    out.push_back({dx, dy});
                }
            }

            return out;
        }

        if (type == "rogue") {
            return {
                { 1,  0},
                {-1,  0},
                { 0,  1},
                { 0, -1},
            };
        }

        if (type == "archer") {
            for (int dx = -2; dx <= 2; ++dx) {
                for (int dy = -2; dy <= 2; ++dy) {
                    if (combat_grid::inArcherRange({0, 0}, {dx, dy})) {
                        out.push_back({dx, dy});
                    }
                }
            }

            return out;
        }

        return out;
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
