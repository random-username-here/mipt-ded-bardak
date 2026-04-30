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
static const int SLASH_FRAMES = 4;
static const int SLASH_SIZE   = 32;

class Atlas {
    Texture2D m_tex;

public:
    Atlas() {
        m_tex.id      = 0;
        m_tex.width   = 0;
        m_tex.height  = 0;
        m_tex.mipmaps = 0;
        m_tex.format  = 0;
    }

    ~Atlas() {
        unload();
    }

    bool loaded() const {
        return m_tex.id != 0;
    }

    Texture2D texture() const {
        return m_tex;
    }

    void load() {
        const char *path = VIS_TILESET_PATH;

        if (!FileExists(path)) return;

        Texture2D tex = LoadTexture(path);
        if (tex.id != 0) {
            m_tex = tex;
            SetTextureFilter(m_tex, TEXTURE_FILTER_POINT);
            return;
        }
    }

    void unload() {
        if (m_tex.id != 0) {
            UnloadTexture(m_tex);
            m_tex.id = 0;
        }
    }

    Rectangle spriteRect(int row, int col) const {
        Rectangle r;
        r.x      = static_cast<float>(col * SPRITE_SIZE);
        r.y      = static_cast<float>(row * SPRITE_SIZE);
        r.width  = static_cast<float>(      SPRITE_SIZE);
        r.height = static_cast<float>(      SPRITE_SIZE);
        return r;
    }

    Rectangle slashRect(int frame) const {
        frame = std::max(0, std::min(SLASH_FRAMES - 1, frame));

        Rectangle r;
        r.x      = static_cast<float>((frame * 2) * SPRITE_SIZE);
        r.y      = static_cast<float>(         3  * SPRITE_SIZE);
        r.width  = static_cast<float>(SLASH_SIZE);
        r.height = static_cast<float>(SLASH_SIZE);
        return r;
    }
};

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
            "%dx%d  units:%zu  bodies:%zu  tick:%llu",
            snap.w,
            snap.h,
            world.units()  .size(),
            world.corpses().size(),
            static_cast<unsigned long long>(snap.tick)
        );

        DrawText(info, 180, 24, 16, Color{170, 170, 170, 255});

        if (!atlas.loaded()) {
            DrawText(
                "tilesheet missing",
                20,
                GetScreenHeight() - 28,
                16,
                Color{230, 120, 120, 255}
            );
        }

        drawTiles  (snap,            cam, atlas);
        drawCorpses(world.corpses(), cam, atlas);

        const std::unordered_map<size_t, VisualUnit> &units = world.units();

        for (auto unit : units) {
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

    static void drawSprite(const Atlas &atlas, int row, int col, float x, float y, float size) {
        Rectangle src = atlas.spriteRect(row, col);
        Rectangle dst = Rectangle{x, y, size, size};

        DrawTexturePro(
            atlas.texture(),
            src,
            dst,
            Vector2{0.0f, 0.0f},
            0.0f,
            WHITE
        );
    }

    static void drawSlashSprite(
        const Atlas &atlas,
        int frame,
        Direction dir,
        float unitX,
        float unitY,
        float tile)
    {
        Rectangle src = atlas.slashRect(frame);

        Vec2f dv = DirectionUtil::vector(dir);

        const float unitCx = unitX + tile * 0.5f;
        const float unitCy = unitY + tile * 0.5f;

        const float size = tile * 2.0f;
        const float cx = unitCx + dv.x * tile * 0.55f;
        const float cy = unitCy + dv.y * tile * 0.55f;

        Rectangle dst = Rectangle{
            cx,
            cy,
            size,
            size
        };

        DrawTexturePro(
            atlas.texture(),
            src,
            dst,
            Vector2{size * 0.5f, size * 0.5f},
            DirectionUtil::rotation(dir),
            WHITE
        );
    }

    void drawTiles(const WorldSnap &snap, const Camera &cam, const Atlas &atlas) {
        for (int y = 0; y < snap.h; ++y) {
            for (int x = 0; x < snap.w; ++x) {
                const size_t idx = static_cast<size_t>(y * snap.w + x);
                const bool walk = snap.walkable[idx];

                Vector2 pos = cam.tileToScreen(static_cast<float>(x), static_cast<float>(y));

                if (atlas.loaded()) {
                    drawSprite(atlas, 0, walk ? 0 : 1, pos.x, pos.y, cam.tile());
                } else {
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
    }

    void drawCorpses(const std::vector<Corpse> &corpses, const Camera &cam, const Atlas &atlas) {
        for (size_t i = 0; i < corpses.size(); ++i) {
            const Corpse &c = corpses[i];
            Vector2 pos = cam.tileToScreen(static_cast<float>(c.x), static_cast<float>(c.y));

            if (atlas.loaded()) {
                drawSprite(atlas, 0, 2, pos.x, pos.y, cam.tile());
            } else {
                DrawCircleV(
                    Vector2{pos.x + cam.tile() * 0.5f, pos.y + cam.tile() * 0.5f},
                    std::max(3.0f, cam.tile() * 0.25f),
                    Color{110, 30, 30, 255}
                );
            }
        }
    }

    void drawUnit(const VisualUnit &u, const Camera &cam, const Atlas &atlas, double now) {
        const bool attacking = u.attacking(now);

        Vec2f rp = u.renderPos(now);
        Vec2f dv = DirectionUtil::vector(u.dir());

        const float nudge = attacking ? u.attackNudge(now, cam.tile()) : 0.0f;

        Vector2 pos = cam.tileToScreen(rp.x, rp.y);
        pos.x += dv.x * nudge;
        pos.y += dv.y * nudge;

        std::cerr << "Current unit assetId: " << u.assetId() << "\n";

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
                attacking ? Color{255, 160, 80, 255} : Color{90, 170, 255, 255}
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

    void drawSlashes(
        const std::vector<SlashParticle> &slashes,
        const Camera &cam,
        const Atlas &atlas,
        double now)
    {
        for (size_t i = 0; i < slashes.size(); ++i) {
            const SlashParticle &p = slashes[i];

            if (!p.time.active(now)) continue;

            const float t = p.time.linear(now);
            int frame = static_cast<int>(std::floor(t * SLASH_FRAMES));
            frame = std::max(0, std::min(SLASH_FRAMES - 1, frame));

            Vector2 pos = cam.tileToScreen(p.x, p.y);

            if (atlas.loaded()) {
                drawSlashSprite(atlas, frame, p.dir, pos.x, pos.y, cam.tile());
            } else {
                Vec2f dv = DirectionUtil::vector(p.dir);

                const float cx = pos.x + cam.tile() * 0.5f + dv.x * cam.tile() * 0.55f;
                const float cy = pos.y + cam.tile() * 0.5f + dv.y * cam.tile() * 0.55f;

                DrawCircleLines(
                    static_cast<int>(cx),
                    static_cast<int>(cy),
                    cam.tile() * (0.35f + 0.25f * t),
                    Color{255, 210, 100, 255}
                );
            }
        }
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
