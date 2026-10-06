#include "aixlog.hpp"
#include "imgui.h"
#include "./debug_wins.hpp"
#include <vector>

static std::vector<std::pair<AixLog::Metadata, std::string>> l_logMessages;

void attachGuiLogHandler() {
    AixLog::Log::instance().add_logsink<AixLog::SinkCallback>(
        AixLog::Severity::debug, [](const AixLog::Metadata &md, const std::string &msg){
            l_logMessages.push_back({md, msg});
        }
    );
}

void ansiText(std::string_view t) {
    size_t start = 0, end = 0;
    ImVec4 color = ImVec4(1, 1, 1, 1);
    size_t i = 0;
    bool prev = false;
    while (i <= t.size()) {
        if (i == t.size() || t[i] == '\x1b') {
            if (start != end) {
                // flush
                if (prev) ImGui::SameLine(0, 0);
                prev = true;
                ImGui::TextColored(color, "%.*s", end - start, t.data() + start);
            }
            ++i;
            if (i >= t.size()) break;
            if (t[i] != '[') continue;
            ++i; 
            int code = 0;
            while (i < t.size() && isdigit(t[i])) {
                code = code * 10 + (t[i] - '0');
                ++i;
            }
            if (i == t.size()) break;
            char cmd = t[i];
            ++i;
            if (cmd == 'm') {
                switch (code) {
                    case 0: color = ImVec4(1, 1, 1, 1); break;
                    case 90: color = ImVec4(0.5, 0.5, 0.5, 1); break;
                    case 91: color = ImVec4(1, 0.5, 0.5, 1); break;
                    case 92: color = ImVec4(0.5, 1, 0.5, 1); break;
                    case 93: color = ImVec4(1, 1, 0.5, 1); break;
                    case 94: color = ImVec4(0.5, 0.5, 1, 1); break;
                    case 95: color = ImVec4(1, 0.5, 1, 1); break;
                    case 96: color = ImVec4(0.5, 1, 1, 1); break;
                    case 97: color = ImVec4(1, 1, 1, 1); break;
                }
            }
            start = end = i;
        } else {
            ++end;
            ++i;
        }
    }
}

void logWindow(bool *show) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::Begin("Logs browser", show);
    if (ImGui::BeginTable("logs", 4, ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX | ImGuiTableFlags_SizingFixedFit, 
        ImVec2(std::max<int>(ImGui::GetWindowWidth() - 20, 10), std::max<int>(ImGui::GetWindowHeight() - 50, 10)))
    ) {
        ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_None, 30);
        ImGui::TableSetupColumn("Tag", ImGuiTableColumnFlags_None, 100);
        ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_None, 400);

        ImGuiListClipper clipper;
        clipper.Begin(l_logMessages.size());
        while (clipper.Step()) {
            for (int r = clipper.DisplayStart; r < clipper.DisplayEnd; ++r) {
                ImGui::TableNextRow();
                const auto &l = l_logMessages[r];
                ImGui::TableNextColumn();
                ImVec4 sevColor; std::string_view sevText;
                switch(l.first.severity) {
                    case AixLog::Severity::trace: sevColor = ImVec4(0.2, 0.2, 0.2, 1); sevText = "[t]"; break;
                    case AixLog::Severity::debug: sevColor = ImVec4(0.4, 0.4, 0.4, 1); sevText = "[d]"; break;
                    case AixLog::Severity::info:  sevColor = ImVec4(0.6, 0.6, 150, 1); sevText = "[i]"; break;
                    case AixLog::Severity::notice: sevColor = ImVec4(0.4, 0.4, 1, 1); sevText = "[n]"; break;
                    case AixLog::Severity::warning: sevColor = ImVec4(1, 1, 0.4, 1); sevText = "[!]"; break;
                    case AixLog::Severity::error: sevColor = ImVec4(1, 0.4, 0.4, 1); sevText = "[x]"; break;
                    case AixLog::Severity::fatal: sevColor = ImVec4(1, 0, 0, 1); sevText = "[#]"; break;
                }
                ImGui::TextColored(sevColor, "%s", sevText.data());
                ImGui::TableNextColumn();

                size_t hash = std::hash<std::string>{}(l.first.tag.text);
                ImVec4 tagColor = ImVec4( 
                    (hash % 7) / 7.0 * 0.8 + 0.2, 
                    (hash / 7 % 7) / 7.0 * 0.8 + 0.2,
                    (hash / 49 % 7) / 7.0 * 0.8 + 0.2,
                    1
                );
                ImGui::TextColored(tagColor, "%s", l.first.tag.text.data());
                ImGui::TableSetColumnIndex(2);
                ansiText(l.second);
            }
        }
        ImGui::EndTable();
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void syncvWindow(syncv::SyncvClient &client, bool *show) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::Begin("Syncvars", show);
    if (ImGui::BeginTable("syncvars", 4, ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX | ImGuiTableFlags_SizingFixedFit, 
        ImVec2(std::max<int>(ImGui::GetWindowWidth() - 20, 10), std::max<int>(ImGui::GetWindowHeight() - 50, 10)))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Id", ImGuiTableColumnFlags_None, 40);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_None, 150);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None, 150);
        ImGui::TableHeadersRow();

        client.forEachVar([](bmsg::Id scope, bmsg::Id idx, const syncv::SyncvClient::VarInfo &vi){
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%u : %u", scope, idx);
            ImGui::TableNextColumn();
            if (vi.type == syncv::VarType::DOUBLE)
                ImGui::TextColored(ImVec4(1, 0.5, 1, 1), "%f", vi.val_dbl());
            else
                ImGui::TextColored(ImVec4(1, 1, 0.5, 1), "%lld", vi.val_i64());
            ImGui::TableNextColumn();
            ImGui::Text("%s", vi.name.data());
        });
        ImGui::EndTable();
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
