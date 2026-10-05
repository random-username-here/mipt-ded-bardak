#pragma once
#include <string_view>
#include "syncv/client.hpp"

void attachGuiLogHandler();
void logWindow(bool *show);
void syncvWindow(syncv::SyncvClient &client, bool *show);

void ansiText(std::string_view t);
