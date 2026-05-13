#include "hunter_proto.hpp"
#include "client-core/base/client_base.hpp"
#include "unit_ai.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <unordered_set>

class HunterClient : public ClientBase {
    bool    m_alive = true;
    int32_t m_hp = 0;
    UnitAi  m_ai{AiRangeKind::ArcherCircle};
    std::unordered_set<std::string> m_items;
    std::unordered_set<std::string> m_abilities;
    uint32_t m_markedEnemy = 0;

public:
    HunterClient(const std::string &ini)
        : ClientBase(ini)
    {
        registerOnPrefix("hunter", [this](const PanFrame &frame) {
            return handleHunterFrame(frame);
        });
    }

private:
    std::string_view roleName() const override
    {
        return "hunter";
    }

    bool keepRunning() const override
    {
        return m_alive;
    }

    bool handleHunterFrame(const PanFrame &frame)
    {
        const std::string raw = frame.rawMessage();
        bmsg::RawMessage msg(raw);
        const std::string_view type = frame.type();

        if (type == "tick") {
            if (!bmsg::SV_hunter_tick::decode(msg)) {
                return false;
            }

            return actOnTick();
        }

        if (type == "hp") {
            const auto hp = bmsg::SV_hunter_hp::decode(msg);
            if (!hp) {
                return false;
            }

            m_hp = hp->val;
            m_alive = m_hp > 0;
            return m_alive;
        }

        if (type == "at") {
            const auto at = bmsg::SV_hunter_at::decode(msg);
            if (!at) {
                return false;
            }

            m_ai.self = {at->x, at->y};
            m_ai.haveSelf = true;
            return true;
        }

        if (type == "root") {
            const auto root = bmsg::SV_hunter_root::decode(msg);
            if (!root) {
                return false;
            }

            m_ai.roots.push_back({{root->x, root->y}, root->who, "root"});
            return true;
        }

        if (type == "enemy") {
            const auto enemy = bmsg::SV_hunter_enemy::decode(msg);
            if (!enemy) {
                return false;
            }

            m_ai.enemies.push_back({
                {enemy->x, enemy->y},
                enemy->who,
                std::string(std::string_view(enemy->kind))
            });
            return true;
        }

        if (type == "wall") {
            const auto wall = bmsg::SV_hunter_wall::decode(msg);
            if (!wall) {
                return false;
            }

            m_ai.walls.insert({wall->x, wall->y});
            return true;
        }

        if (type == "item") {
            const auto item = bmsg::SV_hunter_item::decode(msg);
            if (!item) {
                return false;
            }

            m_items.insert(std::string(item->id));
            return true;
        }

        if (type == "ability") {
            const auto ability = bmsg::SV_hunter_ability::decode(msg);
            if (!ability) {
                return false;
            }

            m_abilities.insert(std::string(ability->id));
            return true;
        }

        return true;
    }

    bool actOnTick()
    {
        if (!m_alive) {
            return false;
        }

        if (!m_ai.haveSelf) {
            m_ai.clearVisible();
            return true;
        }

        if (tryUseOnEnemy()) {
            m_ai.clearVisible();
            return m_alive;
        }

        if (tryMoveTowardEnemy()) {
            m_ai.clearVisible();
            return m_alive;
        }

        if (tryUseOnRoot()) {
            m_ai.clearVisible();
            return m_alive;
        }

        if (tryMoveTowardRoot()) {
            m_ai.clearVisible();
            return m_alive;
        }

        const bool ok = wander();
        m_ai.clearVisible();
        return ok;
    }

    bool tryUseOnEnemy()
    {
        const auto enemy = m_ai.firstEnemyInRange();
        if (!enemy) {
            return false;
        }

        if (m_abilities.count("mark") != 0 && m_markedEnemy != enemy->id) {
            if (sendUse("mark", enemy->id)) {
                m_markedEnemy = enemy->id;
                return true;
            }
        }

        if (m_abilities.count("volley") == 0) {
            return false;
        }
        if (sendUse("volley", enemy->id)) {
            m_markedEnemy = 0;
            return true;
        }
        return false;
    }

    bool tryUseOnRoot()
    {
        if (m_abilities.count("volley") == 0) {
            return false;
        }

        const auto root = m_ai.firstRootInRange();
        if (!root) {
            return false;
        }

        return sendUse("volley", root->id);
    }

    bool tryMoveTowardEnemy()
    {
        const auto step = m_ai.stepTowardEnemyRange();
        if (!step) {
            return false;
        }

        return sendMove(*step);
    }

    bool tryMoveTowardRoot()
    {
        const auto step = m_ai.stepTowardRootRange();
        if (!step) {
            return false;
        }

        return sendMove(*step);
    }

    bool wander()
    {
        const auto step = m_ai.randomStep();
        if (!step) {
            return true;
        }

        return sendMove(*step);
    }

    bool sendUse(std::string_view ability, uint32_t target)
    {
        if (!sendMessage(bmsg::CL_hunter_use{ability, target})) {
            std::cerr << "send hunter use failed\n";
            m_alive = false;
            return false;
        }

        return true;
    }

    bool sendMove(AiPos step)
    {
        if (step.x == 0 && step.y == 0) {
            return false;
        }

        if (!sendMessage(bmsg::CL_hunter_move{
                static_cast<int8_t>(step.x),
                static_cast<int8_t>(step.y)
            })) {
            std::cerr << "send hunter move failed\n";
            m_alive = false;
            return false;
        }

        return true;
    }
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " INI\n";
        return 1;
    }

    try {
        return HunterClient(argv[1]).run() ? 0 : 1;
    }
    catch (const std::exception &err) {
        std::cerr << err.what() << '\n';
        return 1;
    }
}
