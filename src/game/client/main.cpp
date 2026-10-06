#include "raylib_clean.h"
#include "rlImGui.h"
#include "imgui.h"
#include "aixlog.hpp"
#include "./debug_wins.hpp"
#include "MSVA_client.hpp"
#include "imgui_stdlib.h"
#include "syncv/client.hpp"
#include "gameloop.hpp"

msva::MsvaClient g_client;
syncv::SyncvClient g_syncvars;

static bool l_logsOpened = false;
static bool l_demoOpened = false;
static bool l_syncvOpened = false;
static std::string l_host;

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
    switch (g_client.state()) {
        case msva::MsvaClientState::DISCONNECTED: ImGui::TextColored(ImVec4(1, 0.4, 0.4, 1), "Disconnected"); break;
        case msva::MsvaClientState::CONNECTING: ImGui::TextColored(ImVec4(0.4, 0.4, 1, 1), "Connecting"); break;
        case msva::MsvaClientState::CONNECTED: ImGui::TextColored(ImVec4(0.4, 1, 0.4, 1), "Connected"); break;
    }

    ImGui::BeginDisabled(g_client.state() != msva::MsvaClientState::DISCONNECTED);
    ImGui::InputTextWithHint("Address", "localhost:3000", &l_host);
    if (ImGui::Button("Connect!")) {
        LOG(NOTICE, "game") << "Connecting!\n";
        if (g_client.connect(l_host))
            beginGame();
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(g_client.state() != msva::MsvaClientState::CONNECTED);
    if (ImGui::Button("Disconnect"))
        g_client.disconnect();
    ImGui::EndDisabled();

    if (g_client.state() == msva::MsvaClientState::CONNECTED) {
        ImGui::Text("Ping : %lld ns", g_client.delay());
        ImGui::Text("Clock offset : %lld ns", g_client.delta());
    }
    
    ImGui::End();
}

int main() {
    AixLog::Log::init<AixLog::SinkCout>(AixLog::Severity::trace);
    attachGuiLogHandler();

    LOG(NOTICE, "game") << "Application starting!\n";

    InitWindow(1280, 720, "Flightsim");
    SetTargetFPS(60);
    rlImGuiSetup(true);

    g_syncvars.attach(g_client);
    g_client.loadProtoDefs("../");
    g_client.registerHandler("game", [](bmsg::RawMessage msg){
        if (g_client.state() == msva::MsvaClientState::CONNECTED)
            gameMessage(msg);
    });
    
    LOG(NOTICE, "game") << "Main loop begin\n";

    auto prevState = msva::MsvaClientState::DISCONNECTED;
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(Color { 0, 0, 0, 255 });
        rlImGuiBegin();
        if (g_client.state() == msva::MsvaClientState::CONNECTED) {
            gameDraw();
            gameUi();
        }
        l_debugMenu();
        if (l_demoOpened)
            ImGui::ShowDemoWindow(&l_demoOpened);
        if (l_logsOpened)
            logWindow(&l_logsOpened);
        if (l_syncvOpened)
            syncvWindow(g_syncvars, &l_syncvOpened);
        rlImGuiEnd();
        EndDrawing();
        if (prevState == msva::MsvaClientState::CONNECTED && g_client.state() == msva::MsvaClientState::DISCONNECTED)
            quitGame();
        prevState = g_client.state();
        if (g_client.state() == msva::MsvaClientState::CONNECTED) {
            gameUpdate();
            g_client.poll();
        }
    }
    rlImGuiShutdown();
    CloseWindow();
    return 0;
}
