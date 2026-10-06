#include "gameloop.hpp"
#include "defs.hpp"
#include "globals.hpp"
#include "raylib.h"
#include <cmath>
#include "proto.hpp"

struct PlayerInfo {};

static std::unordered_map<bmsg::Id, PlayerInfo> l_players;

void beginGame() {
}

void gameUi() {

}

static void l_drawPlayer(bmsg::Id id) {
    Vector3 pos = {
        .x = g_syncvars.getFloat(id, SYNCV_POS_X),
        .y = g_syncvars.getFloat(id, SYNCV_POS_Y),
        .z = g_syncvars.getFloat(id, SYNCV_POS_Z)
    };
    DrawCylinder(pos, 0.3, 0.3, 1.4, , Color color)
}

void gameDraw() {
    auto id = g_client.id();
    Vector3 pos = {
        .x = g_syncvars.getFloat(id, SYNCV_POS_X),
        .y = g_syncvars.getFloat(id, SYNCV_POS_Y) + 1.4f, // we are not on the floor
        .z = g_syncvars.getFloat(id, SYNCV_POS_Z)
    };
    float azim = g_syncvars.getFloat(id, SYNCV_AZIMUTH);
    float elev = g_syncvars.getFloat(id, SYNCV_ELEV);
    Vector3 to = {
        .x = pos.x + std::cos(azim) * std::cos(elev),
        .y = pos.y + std::sin(elev),
        .z = pos.z + std::sin(azim) * std::cos(elev)
    };
    Camera3D camera = {
        .position = pos,
        .target = to,
        .up = { .x = 0, .y = 1, .z = 0 },
        .fovy = 45,
        .projection = CAMERA_PERSPECTIVE
    };

    BeginMode3D(camera);
    DrawGrid(40, 1.0f);

    for (const auto &[pid, data] : l_players) {
        if (pid == id) continue;
        l_drawPlayer(pid);
    }

    EndMode3D();
}

void gameUpdate() {
}

void gameMessage(bmsg::RawMessage msg) {
    if (msg.header()->pref != "game")
        return;
    if (msg.header()->type == "spawn") {
        auto cmd = bmsg::SV_game_spawn::decode(msg);
        if (!cmd) return;
        l_players[cmd->id] = {};
    } else if (msg.header()->type == "despawn") {
        auto cmd = bmsg::SV_game_despawn::decode(msg);
        if (!cmd) return;
        l_players.erase(cmd->id);
    }
}

void quitGame() {
    g_syncvars.forceClear();
    l_players.clear();
}
