#include "Map.hpp"
#include "Timer.hpp"
#include "modlib_manager.hpp"
#include "modlib_mod.hpp"

#include "snapshot.hpp"
#include "visual.hpp"
#include "render.hpp"

#include <raylib.h>

#include <atomic>
#include <mutex>
#include <string_view>
#include <thread>

class VisMod final : public Mod {
    modlib::Map   *m_map;
    modlib::Timer *m_timer;

    vis::Snapshotter m_snapshotter;

    std::mutex     m_snapLock;
    vis::WorldSnap m_snap;

    std::thread       m_renderThread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_threadStarted;

public:
    VisMod()
        : m_map(NULL)
        , m_timer(NULL)
        , m_snapshotter()
        , m_snapLock()
        , m_snap()
        , m_renderThread()
        , m_running(false)
        , m_threadStarted(false)
    {}

    ~VisMod() override {
        stopRenderer();
    }

    std::string_view id() const override {
        return "ashww.bardak.vis.raylib";
    }

    std::string_view brief() const override {
        return "Raylib map visualizer";
    }

    ModVersion version() const override {
        return ModVersion(0, 2, 0);
    }

    void onResolveDeps(ModManager *mm) override {
        m_map   = mm->anyOfType<modlib::Map>();
        m_timer = mm->anyOfType<modlib::Timer>();

        if (!m_map) {
            throw ModManager::Error("ashww.bardak.vis.raylib: Map module not found");
        }

        if (!m_timer) {
            throw ModManager::Error("ashww.bardak.vis.raylib: Timer module not found");
        }
    }

    void onDepsResolved(ModManager *) override {
        snapshot();

        m_running       = true;
        m_renderThread  = std::thread(&VisMod::renderLoop, this);
        m_threadStarted = true;

        scheduleSnapshot();
    }

    void onBeforeCleanup(ModManager *) override {
        stopRenderer();
    }

private:
    void scheduleSnapshot() {
        if (!m_running || !m_timer) return;

        m_timer->setTimer(
            1,
            [this]() {
                if (!m_running) return;
                snapshot();
                scheduleSnapshot();
            },
            modlib::Timer::Stage::ON_UPDATE_DONE
        );
    }

    void snapshot() {
        vis::WorldSnap next = m_snapshotter.capture(m_map);

        std::lock_guard<std::mutex> lock(m_snapLock);

        if (next.valid) {
            next.tick = m_snap.tick + 1;
        }

        m_snap = next;
    }

    vis::WorldSnap copySnapshot() {
        std::lock_guard<std::mutex> lock(m_snapLock);
        return m_snap;
    }

    void stopRenderer() {
        m_running = false;

        if (m_threadStarted && m_renderThread.joinable()) {
            m_renderThread.join();
        }

        m_threadStarted = false;
    }

    void renderLoop() {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
        InitWindow(1000, 800, "bardak / map");
        SetTargetFPS(60);

        vis::Atlas atlas;
        atlas.load();

        vis::VisualWorld world;
        vis::Renderer renderer;

        uint64_t lastAppliedTick = 0;
        double   lastSnapTime    = GetTime();
        double   tickSeconds     = 1.0;

        while (m_running && !WindowShouldClose()) {
            vis::WorldSnap snap = copySnapshot();
            const double   now  = GetTime();

            if (snap.valid && snap.tick != lastAppliedTick) {
                if (lastAppliedTick != 0 && snap.tick > lastAppliedTick) {
                    const double dt = now - lastSnapTime;
                    const double perTick = dt / static_cast<double>(snap.tick - lastAppliedTick);

                    if (perTick > 0.05 && perTick < 10.0) {
                        tickSeconds = perTick;
                    }
                }

                world.applySnapshot(snap, now, tickSeconds);

                lastAppliedTick = snap.tick;
                lastSnapTime = now;
            }

            world.update(now);

            BeginDrawing();
            ClearBackground(Color{18, 18, 20, 255});

            if (!snap.valid) {
                DrawText("no map", 20, 20, 24, Color{230, 230, 230, 255});
                EndDrawing();
                continue;
            }

            renderer.draw(snap, world, atlas, now);

            EndDrawing();
        }

        atlas.unload();

        if (IsWindowReady()) {
            CloseWindow();
        }

        m_running = false;
    }
};

extern "C" Mod *modlib_create(ModManager *) {
    return new VisMod();
}
