#include "MSVA.hpp"
#include "aixlog.hpp"
#include "defs.hpp"
#include "modlib_manager.hpp"
#include "syncv/server.hpp"
#include "HandmadeMath.h"
#include "proto.hpp"
#include <cstdarg>
#include <cstdio>

const double SPEED = 5; // m/s

enum Axis {
    FORWARD, BACK, LEFT, RIGHT,
    NUM_AXIS
};

struct PlayerData {
    HMM_Vec3 pos;
    double azimuth, elevation;
    bool moving[NUM_AXIS];
    bool disconnecting = false;
    std::string name;
};

std::string l_sprintf(const char *fmt, ...) {
    va_list args, copy;
    va_start(args, fmt);
    va_copy(copy, args);
    int n = vsnprintf(nullptr, 0, fmt, copy);
    std::string s;
    s.resize(n);
    vsnprintf(s.data(), s.size()+1, fmt, args);
    va_end(args);
    va_end(copy);
    return s;
}

class GameServer : public msva::MsvaModule {
public:

    std::string_view id() const override { return "isd.game"; };
    std::string_view brief() const override { return "A game example"; };
    ModVersion version() const override { return ModVersion(0, 0, 1); }

    syncv::SyncvModule *m_sv;

    int64_t m_prevUpdate = 0, m_start = 0;
    int64_t m_minDt = 10000000; // 10ms

    std::unordered_map<msva::MsvaUser*, PlayerData> m_data;

    void onResolveDeps(ModManager *mm) override {
        m_sv = mm->anyOfType<syncv::SyncvModule>();
    }

    void onDepsResolved(ModManager *) override {
    }
    
    void onSetup(msva::MsvaServer *s) override {
        s->registerPrefix("game", this);
    }
    
    void onConnect(msva::MsvaUser *u) override {
        m_data[u] = PlayerData {
            .pos = { 0, 0, 0 },
            .name = u->addr_str()
        };
        m_sv->createVar(u->id(), SYNCV_POS_X, syncv::VarType::DOUBLE, true, l_sprintf("player/%d/x", u->id()));
        m_sv->createVar(u->id(), SYNCV_POS_Y, syncv::VarType::DOUBLE, true, l_sprintf("player/%d/y", u->id()));
        m_sv->createVar(u->id(), SYNCV_POS_Z, syncv::VarType::DOUBLE, true, l_sprintf("player/%d/z", u->id()));
        m_sv->createVar(u->id(), SYNCV_AZIMUTH, syncv::VarType::DOUBLE, true, l_sprintf("player/%d/azim", u->id()));
        m_sv->createVar(u->id(), SYNCV_ELEV, syncv::VarType::DOUBLE, true, l_sprintf("player/%d/elev", u->id()));

        auto name = u->addr_str();
        server()->forAllClients([this, u, &name](msva::MsvaUser *to){
            if (to != u) // do not double send
                u->send(bmsg::SV_game_spawn { to->id(), m_data[to].name });
            to->send(bmsg::SV_game_spawn { u->id(), name });
        });
    };

    void onDisconnect(msva::MsvaUser *u) override {
        m_data[u].disconnecting = true;
        m_sv->removeVar(u->id(), SYNCV_POS_X);
        m_sv->removeVar(u->id(), SYNCV_POS_Y);
        m_sv->removeVar(u->id(), SYNCV_POS_Z);
        m_sv->removeVar(u->id(), SYNCV_AZIMUTH);
        m_sv->removeVar(u->id(), SYNCV_ELEV);
        server()->forAllClients([u](msva::MsvaUser *to){
            to->send(bmsg::SV_game_despawn { u->id() });
        });
    };

    double absTime() const {
        return (msva::nsTime() - m_start) / (long double) 1e9;
    }

    void l_handleCommand(msva::MsvaUser *u, bmsg::Char64 cmd) {
        auto &data = m_data.at(u);
        if(false) {} // just to align

        #define AXIS(NAME, IDX, INVERSE)\
            else if (cmd == "+" NAME) { data.moving[IDX] = true; data.moving[INVERSE] = false; }\
            else if (cmd == "-" NAME) { data.moving[IDX] = false; }

        AXIS("forward", FORWARD, BACK)
        AXIS("back", BACK, FORWARD)
        AXIS("left", LEFT, RIGHT)
        AXIS("right", RIGHT, LEFT)

        #undef AXIS
    }
    
    void onMessage(msva::MsvaUser *u, bmsg::RawMessage msg) override {
        if (msg.header()->pref != "game") return;
        if (msg.header()->type == "command") {
            auto cmd = bmsg::CL_game_command::decode(msg);
            LOG(DEBUG, "game") << "Command " << std::string_view(cmd->header.type) << " from " << u->id() << '\n';
            if (!cmd) return;
            l_handleCommand(u, cmd->action);
        } else if (msg.header()->type == "rotate") {
            auto cmd = bmsg::CL_game_rotate::decode(msg);
            if (!cmd) return;
            auto &data = m_data.at(u);
            data.azimuth = cmd->azimuth;
            data.elevation = cmd->elevation;
        }
    };

    void onIteration() override {
        int64_t now = msva::nsTime();
        if (m_prevUpdate == 0) {
            m_prevUpdate = m_start = now;
            return;
        }
        int64_t delta = now - m_prevUpdate;
        if (delta < m_minDt) return;
        m_prevUpdate = now;
        double dt = delta / (long double) 1e9;
        double abs = ((long double) (now - m_start)) / 1e9;

        for (auto &[user, data] : m_data) {
            if (data.disconnecting) continue;
            float x = std::cos(data.azimuth), z = std::sin(data.azimuth);
            if (data.moving[FORWARD]) data.pos += HMM_Vec3 {x, 0, z} * SPEED * dt;
            if (data.moving[BACK]) data.pos += HMM_Vec3 {-x, 0, -z} * SPEED * dt;
            if (data.moving[LEFT]) data.pos += HMM_Vec3 {z, 0, -x} * SPEED * dt;
            if (data.moving[RIGHT]) data.pos += HMM_Vec3 {-z, 0, x} * SPEED * dt;

            m_sv->setVar(user->id(), SYNCV_POS_X, data.pos.X);
            m_sv->setVar(user->id(), SYNCV_POS_Y, data.pos.Y);
            m_sv->setVar(user->id(), SYNCV_POS_Z, data.pos.Z);
            m_sv->setVar(user->id(), SYNCV_AZIMUTH, data.azimuth);
            m_sv->setVar(user->id(), SYNCV_ELEV, data.elevation);
        }
    }
};

extern "C" Mod* modlib_create() {
    return new GameServer();
}

