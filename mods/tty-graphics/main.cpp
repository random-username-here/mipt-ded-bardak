/**
 * Server-side renderer, which has some nice features for msva server.
 */
#include "BmServerModule.hpp"
#include "Map.hpp"
#include "Timer.hpp"
#include "modlib_manager.hpp"
#include <iostream>
#include <signal.h>
#include <sstream>
#include <sys/ioctl.h>
#include <unistd.h>
#include "msva_api.hpp"
using namespace modlib;

#define ESC_GRY "\x1b[90m"
#define ESC_RED "\x1b[91m"
#define ESC_GRN "\x1b[92m"
#define ESC_YLW "\x1b[93m"
#define ESC_BLU "\x1b[94m"
#define ESC_MGN "\x1b[95m"
#define ESC_CYN "\x1b[96m"
#define ESC_RST "\x1b[0m"

void enterAlt() {
    std::cout << "\x1b[?25l" // hide cursor
              << "\x1b[?1049h"; // alt buffer
    std::cout.flush();
}

void exitAlt() {
    std::cout << ESC_RST // remove stuck color
              << "\x1b[2J" // clear screen
              << "\x1b[?25h" // show cursor
              << "\x1b[?1049l"; // no alt buffer
    std::cout.flush();
}
void exitAlt_i(int v) { exitAlt(); }

class TTYgraph : public BmServerModule {

    std::string_view id() const override { return "isd.bardak.ttygraph"; };
    std::string_view brief() const override { return "Simple game visualization in terminal, for server-side debugging"; };
    ModVersion version() const override { return ModVersion(0, 0, 1); };

    Map *m_map;
    Timer *m_timer;
    msva::Server *m_msva;
    int w, h, cx, cy;
    std::vector<std::string> m_logs;

    void frameBegin() {
        winsize wsz;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &wsz);
        w = wsz.ws_col;
        h = wsz.ws_row;
        std::cout << "\x1b[2J"; // clear screen
    }
    
    void frameEnd() {
    }

    void putch(int x, int y, char ch) {
        if (x < 0 || y < 0 || x >= w || y >= h) return;
        if (cx != x || cy != y) {
            std::cout << "\x1b[" << y+1 << ";" << x+1 << "H";
        }
        std::cout << ch;
        cx++;
        if (cx == w) cx = 0, cy++;
    }

    int puts(int x, int y, std::string_view str) {
        bool escaping = false;
        for (auto c : str) {
            if (c == '\n') continue;
            if (c == '\x1b') escaping = true;
            if (escaping) std::cout << c;
            else putch(x++, y, c);
            if (escaping && c == 'm') escaping = false;
        }
        return x;
    }

    template<typename T>
    std::string stringify(const T& x) {
        return std::to_string(x);
    }

    std::string stringify(const char *c) { return std::string(c); }
    std::string stringify(const std::string &c) { return std::string(c); }

    template<typename ...Args>
    int put(int x, int y, const Args &...args) {
        ([&]{
            x = puts(x, y, stringify(args));
        }(), ...);
        return x;
    }

    void gamemap(int x, int y) {
        for (int i = 0; i < m_map->size().y; ++i) {
            for (int j = 0; j < m_map->size().x; ++j) {
                const char *clr = "", *chr = "?";
                auto tile = m_map->at({j, i});
                if (!tile->units().empty()) {
                    clr = ESC_RED; // chr = "@";
					// hotfix to see difference between ghost and pacman 
					if (tile->units().size() > 1)
						chr = "?";
					else
						chr = (tile->units()[0]->getAssetId() == 0 ? "0" : "1");
				}
                else if (!(tile->type() == Tile::BasicType::Wall))
                    clr = ESC_RST, chr = "#";
                else
                    clr = ESC_GRY, chr = ".";
                put(x + j, y + i, clr, chr, ESC_RST);
            }
        }
    }

    void stats(int x, int y) {
        std::stringstream ss;
        ss << "Tick " << ESC_YLW << m_timer->getTicksSinceCreation() << ESC_RST;
        put(x, y, ss.str());
    }

    void logs(int x, int y, int n) {
        for (size_t i = 0; i < m_logs.size() && i < n; ++i) {
            put(x, y+n-i-1, m_logs[m_logs.size()-1-i]);
        }
    }

    void render() {
        frameBegin();
        gamemap(w - m_map->size().x - 2, 1);
        stats(w - m_map->size().x - 2, m_map->size().y + 2);
        logs(1, 0, h);
        frameEnd();
        std::cout.flush();
    }

    void timedRender() {
        render();
        m_timer->setTimer(1, [this](){ timedRender(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }

    void onResolveDeps(ModManager *mm) override {
        m_map = mm->anyOfType<Map>();
        m_timer = mm->anyOfType<Timer>();
        if (!m_map) throw ModManager::Error("TTYGraph needs Map to visualize");
        if (!m_timer) throw ModManager::Error("TTYGraph needs a Timer");
    }

    void onDepsResolved(ModManager *mm)  override {
        m_timer->setTimer(1, [this](){ timedRender(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }

    void onSetup(BmServer *server) override {
        m_msva = dynamic_cast<msva::Server*>(server);
        if (m_msva) {
            m_msva->setTTYLogs(false);
            auto pl = m_msva->logCallback();
            m_msva->setLogCallback([this, pl](std::string_view s){
                m_logs.push_back(std::string(s));
                render();
                if (pl) pl(s);
            });
            signal(SIGINT, exitAlt_i);
            signal(SIGABRT, exitAlt_i);
            signal(SIGTERM, exitAlt_i);
            atexit(exitAlt);
            enterAlt();
        }
    }
};

extern "C" Mod *modlib_create(ModManager *mm) {
    return new TTYgraph();
}
