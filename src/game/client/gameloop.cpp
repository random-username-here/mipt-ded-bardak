#include "gameloop.hpp"
#include "binmsg.hpp"
#include "defs.hpp"
#include "globals.hpp"
#include "raylib_clean.h"
#include "aixlog.hpp"
#include <cmath>
#include <iostream>
#include "proto.hpp"

struct PlayerInfo {};

static std::unordered_map<bmsg::Id, PlayerInfo> l_players;
double l_azimuth = 0, l_elev = 0;

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
    DrawCapsuleWires(Vector3 { pos.x, pos.y + 0.3f, pos.z }, Vector3 { pos.x, pos.y + 1.4f, pos.z }, 0.3, 8, 8, Color{255, 0, 0, 255});
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

    for (const auto &[pid, data] : l_players) {
        if (pid == id) continue;
        l_drawPlayer(pid);
    }
    DrawGrid(40, 1.0f);

    EndMode3D();
}

void gameUpdate() {
#define DIR_KEY(KEY, DIR)\
        if (IsKeyPressed(KEY)) g_client.send(bmsg::CL_game_command { "+" DIR });\
        if (IsKeyReleased(KEY)) g_client.send(bmsg::CL_game_command { "-" DIR });

    DIR_KEY(KEY_W, "forward")
    DIR_KEY(KEY_A, "left")
    DIR_KEY(KEY_S, "back")
    DIR_KEY(KEY_D, "right")

#undef DIR_KEY

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !IsCursorHidden())
        DisableCursor();
    if (IsKeyPressed(KEY_ESCAPE))
        ShowCursor();

    if (IsCursorHidden()) {
        Vector2 delta = GetMouseDelta();
        delta.x /= GetScreenWidth();
        delta.y /= GetScreenWidth();
        l_azimuth += delta.x * M_PI * 2;
        l_elev -= delta.y * M_PI * 2; // invert
        if (l_elev > M_PI / 2) l_elev = M_PI/2;
        if (l_elev < -M_PI / 2) l_elev = -M_PI/2;
        g_client.send(bmsg::CL_game_rotate { l_azimuth, l_elev }, bmsg::USE_UDP);
    }
}

void gameMessage(bmsg::RawMessage msg) {
    if (msg.header()->pref != "game")
        return;
    if (msg.header()->type == "spawn") {
        auto cmd = bmsg::SV_game_spawn::decode(msg);
        if (!cmd) return;
        LOG(INFO, "game") << "Spawn player " << cmd->id << '\n';
        l_players[cmd->id] = {};
    } else if (msg.header()->type == "despawn") {
        auto cmd = bmsg::SV_game_despawn::decode(msg);
        if (!cmd) return;
        LOG(INFO, "game") << "Despawn player " << cmd->id << '\n';
        l_players.erase(cmd->id);
    }
}

void quitGame() {
    g_syncvars.forceClear();
    l_players.clear();
    if (IsCursorHidden())
        ShowCursor();
}
