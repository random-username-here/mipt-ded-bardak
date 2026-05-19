#pragma once

#include "brain_bridge.hpp"
#include "client-core/base/client_base.hpp"

#include <string>
#include <string_view>

class KnightScriptAdapter : public ClientBase
{
public:
    KnightScriptAdapter(std::string ini_path, std::string brain_executable, BrainBridge &brain);

private:
    std::string_view roleName() const override;
    bool keepRunning() const override;

    bool handleKnightFrame(const PanFrame &frame);
    bool actOnTick();

    bool sendUse(uint32_t target);
    bool sendMove(int8_t dx, int8_t dy);

    static bool isSlashAbility(std::string_view id);

    BrainBridge &m_brain;
    bool m_alive = true;
};
