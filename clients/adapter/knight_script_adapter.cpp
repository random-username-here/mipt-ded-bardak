#include "knight_script_adapter.hpp"

#include "brain_protocol.hpp"
#include "knight_proto.hpp"

#include <iostream>
#include <string>

KnightScriptAdapter::KnightScriptAdapter(
    std::string ini_path,
    std::string /*brain_executable*/,
    BrainBridge &brain)
    : ClientBase(ini_path)
    , m_brain(brain)
{
    registerOnPrefix("knight", [this](const PanFrame &frame) {
        return handleKnightFrame(frame);
    });
}

std::string_view KnightScriptAdapter::roleName() const
{
    return "knight";
}

bool KnightScriptAdapter::keepRunning() const
{
    return m_alive;
}

bool KnightScriptAdapter::isSlashAbility(std::string_view id)
{
    return id == "slash";
}

bool KnightScriptAdapter::handleKnightFrame(const PanFrame &frame)
{
    const std::string raw = frame.rawMessage();
    bmsg::RawMessage msg(raw);
    const std::string_view type = frame.type();

    if (type == "tick") {
        if (!bmsg::SV_knight_tick::decode(msg)) {
            return false;
        }
        return actOnTick();
    }

    if (type == "hp") {
        const auto hp = bmsg::SV_knight_hp::decode(msg);
        if (!hp) {
            return false;
        }
        m_alive = hp->val > 0;
        if (!m_brain.sendHp(hp->val)) {
            return false;
        }
        return m_alive;
    }

    if (type == "at") {
        const auto at = bmsg::SV_knight_at::decode(msg);
        if (!at) {
            return false;
        }
        return m_brain.sendAt(at->x, at->y);
    }

    if (type == "root") {
        const auto root = bmsg::SV_knight_root::decode(msg);
        if (!root) {
            return false;
        }
        return m_brain.sendRoot(root->x, root->y, root->who);
    }

    if (type == "enemy") {
        const auto enemy = bmsg::SV_knight_enemy::decode(msg);
        if (!enemy) {
            return false;
        }
        return m_brain.sendEnemy(enemy->x, enemy->y, enemy->who);
    }

    if (type == "wall") {
        const auto wall = bmsg::SV_knight_wall::decode(msg);
        if (!wall) {
            return false;
        }
        return m_brain.sendWall(wall->x, wall->y);
    }

    if (type == "item") {
        return bmsg::SV_knight_item::decode(msg).has_value();
    }

    if (type == "ability") {
        const auto ability = bmsg::SV_knight_ability::decode(msg);
        if (!ability) {
            return false;
        }
        if (isSlashAbility(ability->id)) {
            return m_brain.sendAbilitySlash();
        }
        return true;
    }

    return true;
}

bool KnightScriptAdapter::actOnTick()
{
    if (!m_alive) {
        return false;
    }

    if (!m_brain.sendTick()) {
        std::cerr << "brain bridge: send tick failed\n";
        m_alive = false;
        return false;
    }

    BrainBridge::Action action{};
    if (!m_brain.queryAction(action)) {
        std::cerr << "brain bridge: query action failed\n";
        m_alive = false;
        return false;
    }

    if (action.kind == brain_proto::kActStop) {
        m_alive = false;
        return false;
    }

    if (action.kind == brain_proto::kActMove) {
        return sendMove(static_cast<int8_t>(action.dx), static_cast<int8_t>(action.dy));
    }

    if (action.kind == brain_proto::kActUse) {
        return sendUse(action.target);
    }

    return true;
}

bool KnightScriptAdapter::sendUse(uint32_t target)
{
    if (!sendMessage(bmsg::CL_knight_use{"slash", target})) {
        std::cerr << "send knight use failed\n";
        m_alive = false;
        return false;
    }
    return true;
}

bool KnightScriptAdapter::sendMove(int8_t dx, int8_t dy)
{
    if (dx == 0 && dy == 0) {
        return false;
    }

    if (!sendMessage(bmsg::CL_knight_move{dx, dy})) {
        std::cerr << "send knight move failed\n";
        m_alive = false;
        return false;
    }
    return true;
}
