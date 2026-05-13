#include "client-core/base/client_base.hpp"
#include "mage_proto.hpp"
#include "unit_ai.hpp"

#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>

class MageClient : public ClientBase {
    static constexpr int32_t kMageMaxHp = 55;

    bool    m_alive  = true;
    bool    m_last_p = false;
    int32_t m_hp     = 0;
    UnitAi  m_ai{AiRangeKind::MageFlameStar};
    std::unordered_set<std::string> m_items;
    std::unordered_set<std::string> m_abilities;

public:
    MageClient(const std::string &ini)
        : ClientBase(ini)
    {
        registerOnPrefix("mage", [this](const PanFrame &frame) {
            return handleMageFrame(frame);
        });
    }

private:
    std::string_view roleName() const override
    {
        return "mage";
    }

    bool keepRunning() const override
    {
        return m_alive;
    }

    bool handleMageFrame(const PanFrame &frame)
    {
        const std::string raw = frame.rawMessage();
        bmsg::RawMessage msg(raw);
        const std::string_view type = frame.type();

        if (type == "tick") {
            if (!bmsg::SV_mage_tick::decode(msg)) {
                return false;
            }

            return actOnTick();
        }

        if (type == "hp") {
            const auto hp = bmsg::SV_mage_hp::decode(msg);
            if (!hp) {
                return false;
            }

            m_hp    = hp->val;
            m_alive = m_hp > 0;
            return m_alive;
        }

        if (type == "at") {
            const auto at = bmsg::SV_mage_at::decode(msg);
            if (!at) {
                return false;
            }

            m_ai.self = {at->x, at->y};
            m_ai.haveSelf = true;
            return true;
        }

        if (type == "root") {
            const auto root = bmsg::SV_mage_root::decode(msg);
            if (!root) {
                return false;
            }

            m_ai.roots.push_back({{root->x, root->y}, root->who, "root"});
            return true;
        }

        if (type == "enemy") {
            const auto enemy = bmsg::SV_mage_enemy::decode(msg);
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
            const auto wall = bmsg::SV_mage_wall::decode(msg);
            if (!wall) {
                return false;
            }

            m_ai.walls.insert({wall->x, wall->y});
            return true;
        }

        if (type == "item") {
            const auto item = bmsg::SV_mage_item::decode(msg);
            if (!item) {
                return false;
            }

            m_items.insert(std::string(item->id));
            return true;
        }

        if (type == "ability") {
            const auto ability = bmsg::SV_mage_ability::decode(msg);
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

        if (tryHeal()) {
            m_ai.clearVisible();
            return m_alive;
        }

        if (m_last_p && tryFlameEnemy()) {
            m_last_p = false;
            m_ai.clearVisible();
            return m_alive;
        }

        if (tryPlantNearEnemy()) {
            m_last_p = true;
            m_ai.clearVisible();
            return m_alive;
        }

        if (tryMoveTowardEnemy()) {
            m_ai.clearVisible();
            return m_alive;
        }

        const bool ok = wander();
        m_ai.clearVisible();
        return ok;
    }

    bool tryHeal()
    {
        if (m_abilities.count("heal") == 0) {
            return false;
        }

        if (m_hp <= 0 || m_hp >= kMageMaxHp) {
            return false;
        }

        return sendUse("heal", 0, m_ai.self);
    }

    bool tryFlameEnemy()
    {
        if (m_abilities.count("flame") == 0) {
            return false;
        }

        const auto enemy = m_ai.firstEnemyInRange();
        if (!enemy) {
            return false;
        }

        return sendUse("flame", enemy->id, enemy->pos);
    }

    bool tryPlantNearEnemy()
    {
        if (m_abilities.count("plant") == 0) {
            return false;
        }

        const auto enemy = firstEnemyInPlantRange();
        if (!enemy) {
            return false;
        }

        return sendUse("plant", enemy->id, enemy->pos);
    }

    bool tryMoveTowardEnemy()
    {
        const auto step = m_ai.stepTowardEnemyRange();
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

    std::optional<AiSeenEntity> firstEnemyInPlantRange() const
    {
        for (const auto &enemy : m_ai.enemies) {
            if (aiInRange(m_ai.self, enemy.pos, AiRangeKind::ArcherCircle)) {
                return enemy;
            }
        }

        return std::nullopt;
    }

    bool sendUse(std::string_view ability, uint32_t target, AiPos point)
    {
        if (!sendMessage(bmsg::CL_mage_use{ability, target, point.x, point.y})) {
            std::cerr << "send mage use failed\n";
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

        if (!sendMessage(bmsg::CL_mage_move{
                static_cast<int8_t>(step.x),
                static_cast<int8_t>(step.y)
            })) {
            std::cerr << "send mage move failed\n";
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
        return MageClient(argv[1]).run() ? 0 : 1;
    }
    catch (const std::exception &err) {
        std::cerr << err.what() << '\n';
        return 1;
    }
}
