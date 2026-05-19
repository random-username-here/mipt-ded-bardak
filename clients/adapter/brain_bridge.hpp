#pragma once

#include <cstdint>
#include <string>
#include <string_view>

class BrainBridge
{
public:
    struct Action
    {
        int32_t kind = 0;
        int32_t dx = 0;
        int32_t dy = 0;
        uint32_t target = 0;
    };

    BrainBridge(std::string_view host, std::string_view port);
    ~BrainBridge();

    BrainBridge(const BrainBridge &) = delete;
    BrainBridge &operator=(const BrainBridge &) = delete;

    bool listen();
    bool launchScript(std::string_view executable);
    bool acceptScript(int timeout_ms = 10000);

    // When executable is empty, caller starts the brain process manually.
    bool waitForScript(int timeout_ms = 10000);

    bool sendTick();
    bool sendHp(int32_t hp);
    bool sendAt(int32_t x, int32_t y);
    bool sendRoot(int32_t x, int32_t y, uint32_t id);
    bool sendEnemy(int32_t x, int32_t y, uint32_t id);
    bool sendWall(int32_t x, int32_t y);
    bool sendAbilitySlash();

    bool queryAction(Action &out, int timeout_ms = 5000);

    void close();

private:
    bool sendInt(int32_t value);
    bool recvInt(int32_t &value, int timeout_ms);

    bool waitReadable(int timeout_ms);
    bool waitWritable(int timeout_ms);

    std::string m_host;
    std::string m_port;

    int m_listen_fd = -1;
    int m_client_fd = -1;
};
