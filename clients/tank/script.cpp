#include "client-core/base/client_base.hpp"
#include "tank_proto.hpp"
#include "unit_ai.hpp"

#include <exception>
#include <iostream>
#include <optional>
#include <string>

class TankClient : public ClientBase {
    bool    m_alive = true;
    int32_t m_hp = 0;
    UnitAi  m_ai{AiRangeKind::VonNeumann};
    int8_t  m_dir = 1; // 0 - left, 1 - right, 2 - down, 3 - up

public:
    TankClient(const std::string &ini)
        : ClientBase(ini)
    {
        registerOnPrefix("tank", [this](const PanFrame &frame) {
            return handleTankFrame(frame);
        });
    }

private:
    std::string_view roleName() const override
    {
        return "tank";
    }

    bool keepRunning() const override
    {
        return m_alive;
    }

    bool handleTankFrame(const PanFrame &frame)
    {
        const std::string raw = frame.rawMessage();
        bmsg::RawMessage msg(raw);
        const std::string_view type = frame.type();

        if (type == "tick") {
            if (!bmsg::SV_tank_tick::decode(msg)) {
                return false;
            }
            return actOnTick();
        }

        if (type == "hp") {
            const auto hp = bmsg::SV_tank_hp::decode(msg);
            if (!hp) {
                return false;
            }

            m_hp = hp->val;
            m_alive = m_hp > 0;
            return m_alive;
        }

        if (type == "at") {
            const auto at = bmsg::SV_tank_at::decode(msg);
            if (!at) {
                return false;
            }

            m_ai.self = {at->x, at->y};
            m_ai.haveSelf = true;
            return true;
        }

        if (type == "root") {
            const auto root = bmsg::SV_tank_root::decode(msg);
            if (!root) {
                return false;
            }

            m_ai.roots.push_back({{root->x, root->y}, root->who, "root"});
            return true;
        }

        if (type == "enemy") {
            const auto enemy = bmsg::SV_tank_enemy::decode(msg);
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
            const auto wall = bmsg::SV_tank_wall::decode(msg);
            if (!wall) {
                return false;
            }

            m_ai.walls.insert({wall->x, wall->y});
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

        if (tryShootAdjacent()) {
            m_ai.clearVisible();
            return m_alive;
        }

        if (tryMoveTowardEnemyOrRoot()) {
            m_ai.clearVisible();
            return m_alive;
        }

        const bool ok = wander();
        m_ai.clearVisible();
        return ok;
    }

    std::optional<int8_t> desiredDirTo(AiPos target) const
    {
        const int dx = target.x - m_ai.self.x;
        const int dy = target.y - m_ai.self.y;

        if (dx == 0 && dy == 0) {
            return std::nullopt;
        }

        if (aiAbs(dx) >= aiAbs(dy)) {
            if (dx < 0) {
                return static_cast<int8_t>(0);
            }
            if (dx > 0) {
                return static_cast<int8_t>(1);
            }
        }

        if (dy > 0) {
            return static_cast<int8_t>(2);
        }
        if (dy < 0) {
            return static_cast<int8_t>(3);
        }

        return std::nullopt;
    }

    bool tryShootAdjacent()
    {
        for (const auto &enemy : m_ai.enemies) {
            if (aiManhattan(enemy.pos, m_ai.self) != 1) {
                continue;
            }

            const auto want = desiredDirTo(enemy.pos);
            if (!want) {
                continue;
            }

            if (m_dir != *want) {
                return sendRotate(*want);
            }

            return sendShoot();
        }

        return false;
    }

    bool tryMoveTowardEnemyOrRoot()
    {
        std::optional<AiPos> target;
        int32_t bestDist = 0;

        auto consider = [&](AiPos p) {
            const int32_t dist = aiAbs(p.x - m_ai.self.x) + aiAbs(p.y - m_ai.self.y);
            if (!target || dist < bestDist) {
                target = p;
                bestDist = dist;
            }
        };

        for (const auto &e : m_ai.enemies) consider(e.pos);
        for (const auto &r : m_ai.roots)   consider(r.pos);

        if (!target) {
            return false;
        }

        const auto want = desiredDirTo(*target);
        if (want && m_dir != *want) {
            return sendRotate(*want);
        }

        return sendMove();
    }

    bool wander()
    {
        if (sendMove()) {
            return true;
        }

        static constexpr int8_t nextDir[4] = {3, 2, 1, 0};
        return sendRotate(nextDir[m_dir & 3]);
    }

    bool sendRotate(int8_t dir)
    {
        if (!sendMessage(bmsg::CL_tank_rotate{dir})) {
            std::cerr << "send tank rotate failed\n";
            m_alive = false;
            return false;
        }

        m_dir = dir;
        return true;
    }

    bool sendMove()
    {
        if (!sendMessage(bmsg::CL_tank_move{})) {
            std::cerr << "send tank move failed\n";
            m_alive = false;
            return false;
        }
        return true;
    }

    bool sendShoot()
    {
        if (!sendMessage(bmsg::CL_tank_shoot{})) {
            std::cerr << "send tank shoot failed\n";
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
        return TankClient(argv[1]).run() ? 0 : 1;
    }
    catch (const std::exception &err) {
        std::cerr << err.what() << '\n';
        return 1;
    }
}

