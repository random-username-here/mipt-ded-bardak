#include "raylib_clean.h"
#include "rlImGui.h"
#include "imgui.h"
#include "aixlog.hpp"
#include "./debug_wins.h"
#include "MSVA_client.hpp"
#include "imgui_stdlib.h"
#include "syncv/client.hpp"

static bool l_logsOpened = false;
static bool l_demoOpened = false;
static bool l_syncvOpened = false;
static msva::MsvaClient l_client;
static syncv::SyncvClient l_syncvs;
static std::string l_host;

static void l_beginGame() {
    // pass
}

static void l_gameDraw() {
    DrawCircle(
        400 + 300 * l_syncvs.getDouble(0, 0), 
        400 + 300 * l_syncvs.getDouble(0, 1),
        10,
        Color { 128, 128, 255, 255 }
    );
}

static void l_gameLoop() {

}

static void l_quitGame() {
    l_syncvs.forceClear();
}

static void l_debugMenu() {
    ImGui::Begin("Debug");
    
    if (ImGui::Button("ImGUI demo"))
        l_demoOpened = true;
    ImGui::SameLine();
    if (ImGui::Button("Log browser"))
        l_logsOpened = true;
    ImGui::SameLine();
    if (ImGui::Button("Syncvars"))
        l_syncvOpened = true;

    ImGui::Separator();

    ImGui::Text("Connection status: ");
    ImGui::SameLine();
    switch (l_client.state()) {
        case msva::MsvaClientState::DISCONNECTED: ImGui::TextColored(ImVec4(1, 0.4, 0.4, 1), "Disconnected"); break;
        case msva::MsvaClientState::CONNECTING: ImGui::TextColored(ImVec4(0.4, 0.4, 1, 1), "Connecting"); break;
        case msva::MsvaClientState::CONNECTED: ImGui::TextColored(ImVec4(0.4, 1, 0.4, 1), "Connected"); break;
    }

    ImGui::BeginDisabled(l_client.state() != msva::MsvaClientState::DISCONNECTED);
    ImGui::InputTextWithHint("Address", "localhost:3000", &l_host);
    if (ImGui::Button("Connect!")) {
        LOG(NOTICE, "fsim") << "Connecting!\n";
        if (l_client.connect(l_host))
            l_beginGame();
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(l_client.state() != msva::MsvaClientState::CONNECTED);
    if (ImGui::Button("Disconnect"))
        l_client.disconnect();
    ImGui::EndDisabled();

    if (l_client.state() == msva::MsvaClientState::CONNECTED) {
        ImGui::Text("Ping : %lld ns", l_client.delay());
        ImGui::Text("Clock offset : %lld ns", l_client.delta());
    }
    
    ImGui::End();
}

int main() {
    AixLog::Log::init<AixLog::SinkCout>(AixLog::Severity::trace);
    attachGuiLogHandler();

    LOG(NOTICE, "fsim") << "Application starting!\n";

    InitWindow(1280, 720, "Flightsim");
    SetTargetFPS(60);
    rlImGuiSetup(true);

    l_syncvs.attach(l_client);
    l_client.loadProtoDefs("../");
    
    LOG(NOTICE, "fsim") << "Main loop begin\n";

    auto prevState = msva::MsvaClientState::DISCONNECTED;
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(Color { 0, 0, 0, 255 });
        rlImGuiBegin();
        if (l_client.state() == msva::MsvaClientState::CONNECTED)
            l_gameDraw();
        l_debugMenu();
        if (l_demoOpened)
            ImGui::ShowDemoWindow(&l_demoOpened);
        if (l_logsOpened)
            logWindow(&l_logsOpened);
        if (l_syncvOpened)
            syncvWindow(l_syncvs, &l_syncvOpened);
        rlImGuiEnd();
        EndDrawing();
        if (prevState == msva::MsvaClientState::CONNECTED && l_client.state() == msva::MsvaClientState::DISCONNECTED)
            l_quitGame();
        prevState = l_client.state();
        if (l_client.state() == msva::MsvaClientState::CONNECTED) {
            l_gameLoop();
            l_client.poll();
        }
    }
    rlImGuiShutdown();
    CloseWindow();
    return 0;
}
