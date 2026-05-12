#pragma once

#include "visual.hpp"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <cassert>

#ifndef VIS_TILESET_PATH
#define VIS_TILESET_PATH "mods/vis/tilesheet.png"
#endif

namespace vis {

static const int SPRITE_SIZE  = 16;

class Camera {
    float m_ox;
    float m_oy;
    float m_tile;

public:
    Camera()
        : m_ox(0.0f)
        , m_oy(0.0f)
        , m_tile(16.0f)
    {}

    void fit(int screenW, int screenH, int mapW, int mapH) {
        const float margin = 32.0f;
        const float top    = 56.0f;

        const float usableW = std::max(1.0f, static_cast<float>(screenW) - margin * 2.0f);
        const float usableH = std::max(1.0f, static_cast<float>(screenH) - top - margin);

        const float tileW = usableW / static_cast<float>(mapW);
        const float tileH = usableH / static_cast<float>(mapH);

        m_tile = std::max(4.0f, std::floor(std::min(tileW, tileH)));

        const float realMapW = m_tile * static_cast<float>(mapW);
        const float realMapH = m_tile * static_cast<float>(mapH);

        m_ox = std::floor((static_cast<float>(screenW) - realMapW) * 0.5f);
        m_oy = top + std::floor((usableH - realMapH) * 0.5f);
    }

    float tile() const { return m_tile; }
    float ox()   const { return m_ox;   }
    float oy()   const { return m_oy;   }

    Vector2 tileToScreen(float x, float y) const {
        return Vector2{
            m_ox + x * m_tile,
            m_oy + y * m_tile
        };
    }
};

class Renderer {
    modlib::AssetManager * m_assetManager=nullptr;
public:
    explicit Renderer(modlib::AssetManager *assetManager) : m_assetManager(assetManager) { assert(m_assetManager); }

    void draw(const WorldSnap &snap, const VisualWorld &world, const Atlas &atlas, double now) {
        Camera cam;
        cam.fit(GetScreenWidth(), GetScreenHeight(), snap.w, snap.h);

        DrawText("bardak", 20, 18, 24, Color{235, 235, 235, 255});

        char info[128];
        std::snprintf(
            info,
            sizeof(info),
            "%dx%d  entities:%zu  bodies:%zu  tick:%llu",
            snap.w,
            snap.h,
            world.entities()  .size(),
            world.corpses().size(),
            static_cast<unsigned long long>(snap.tick)
        );

        DrawText(info, 180, 24, 16, Color{170, 170, 170, 255});

        drawTiles  (snap,            cam);
        drawCorpses(world.corpses(), cam);

        const std::unordered_map<size_t, VisualUnit> &entities = world.entities();

        for (auto unit : entities) {
            drawUnit(unit.second, cam, now);
        }
    }

private:
    void unloadAssetTextures() {
        for (auto &entry : m_assetTextures) {
            if (entry.second.id != 0) {
                UnloadTexture(entry.second);
            }
        }

        m_assetTextures.clear();
        m_failedAssetTextures.clear();
    }

    Texture2D *textureFor(const modlib::SpriteAsset &sprite) {
        const uint64_t key = sprite.id.as_u64;

        auto loaded = m_assetTextures.find(key);
        if (loaded != m_assetTextures.end()) {
            return &loaded->second;
        }

        drawTiles  (snap,            cam, atlas);
        drawCorpses(world.corpses(), cam, atlas);

        const std::unordered_map<size_t, VisualUnit> &entities = world.entities();

        for (auto unit : entities) {
            drawUnit(unit.second, cam, atlas, now);
        }

        drawSlashes(world.slashes(), cam, atlas, now);
    }

private:
    static Color hpColor(float frac) {
        frac = std::max(0.0f, std::min(1.0f, frac));

        const unsigned char r = static_cast<unsigned char>(255.0f * (1.0f - frac));
        const unsigned char g = static_cast<unsigned char>(220.0f * frac);
        const unsigned char b = 40;

        return Color{r, g, b, 255};
    }

    void drawTiles(const WorldSnap &snap, const Camera &cam) {
        for (int y = 0; y < snap.h; ++y) {
            for (int x = 0; x < snap.w; ++x) {
                const size_t idx = static_cast<size_t>(y * snap.w + x);
                const bool walk = snap.walkable[idx];

                Vector2 pos = cam.tileToScreen(static_cast<float>(x), static_cast<float>(y));

                Color fill = walk
                    ? Color{52, 58, 52, 255}
                    : Color{30, 30, 34, 255};

                DrawRectangleV(pos, Vector2{cam.tile(), cam.tile()}, fill);
                DrawRectangleLines(
                    static_cast<int>(pos.x),
                    static_cast<int>(pos.y),
                    static_cast<int>(cam.tile()),
                    static_cast<int>(cam.tile()),
                    Color{75, 75, 80, 255}
                );
            }
        }
    }

    void drawCorpses(const std::vector<Corpse> &corpses, const Camera &cam) {
        for (size_t i = 0; i < corpses.size(); ++i) {
            const Corpse &c = corpses[i];
            Vector2 pos = cam.tileToScreen(static_cast<float>(c.x), static_cast<float>(c.y));

            DrawCircleV(
                Vector2{pos.x + cam.tile() * 0.5f, pos.y + cam.tile() * 0.5f},
                std::max(3.0f, cam.tile() * 0.25f),
                Color{110, 30, 30, 255}
            );
        }
    }

    void drawUnit(const VisualUnit &u, const Camera &cam, double now) {
        Vec2f rp = u.renderPos(now);

        Vector2 pos = cam.tileToScreen(rp.x, rp.y);
        pos.x += dv.x * nudge;
        pos.y += dv.y * nudge;

        // std::cerr << "Current unit assetId: " << u.assetId() << "\n";

        if (atlas.loaded()) {
            const int row = attacking ? 2 : 1;
            const int col = static_cast<int>(u.dir());

            drawSprite(atlas, row, col, pos.x, pos.y, cam.tile());
        } else {
            const float cx = pos.x + cam.tile() * 0.5f;
            const float cy = pos.y + cam.tile() * 0.5f;
            const float radius = std::max(3.0f, cam.tile() * 0.28f);

            DrawCircleV(
                Vector2{cx, cy},
                radius,
                Color{90, 170, 255, 255}
            );

            DrawCircleLines(
                static_cast<int>(cx),
                static_cast<int>(cy),
                radius,
                Color{220, 240, 255, 255}
            );
        }

        drawHpBar(u, pos.x, pos.y, cam.tile());
    }

    void drawHpBar(const VisualUnit &u, float rx, float ry, float tile)
    {
        const int   hp   = std::max(0, std::min(u.maxHp(), u.hp()));
        const float frac = static_cast<float>(hp) / static_cast<float>(u.maxHp());

        const float outerW = tile * 0.76f;
        const float outerH = std::max(5.0f, tile * 0.13f);

        const float bx = rx + (tile - outerW) * 0.5f;
        const float by = ry - outerH - tile * 0.10f;

        const float pad    = std::max(1.0f, std::floor(tile * 0.035f));
        const float innerX = bx + pad;
        const float innerY = by + pad;
        const float innerW = std::max(0.0f, outerW - pad * 2.0f);
        const float innerH = std::max(1.0f, outerH - pad * 2.0f);

        DrawRectangleV(
            Vector2{bx, by},
            Vector2{outerW, outerH},
            Color{18, 18, 18, 230}
        );

        DrawRectangleV(
            Vector2{innerX, innerY},
            Vector2{innerW, innerH},
            Color{70, 20, 20, 230}
        );

        DrawRectangleV(
            Vector2{innerX, innerY},
            Vector2{innerW * frac, innerH},
            hpColor(frac)
        );

        DrawRectangleLinesEx(
            Rectangle{bx, by, outerW, outerH},
            std::max(1.0f, std::floor(tile * 0.035f)),
            Color{5, 5, 5, 255}
        );
    }
};

} // namespace vis
