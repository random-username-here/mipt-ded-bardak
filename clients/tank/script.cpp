#include "client-core/base/client_base.hpp"
#include "tank_proto.hpp"
#include "unit_ai.hpp"

#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <random>

class TankClient : public ClientBase {
    bool    m_alive = true;
    int32_t m_hp = 0;
    UnitAi  m_ai{AiRangeKind::VonNeumann};
    int8_t  m_dir = 1; 
    std::mt19937 m_gen{std::random_device{}()};

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

        std::uniform_int_distribution<> fireDist(1, 4);
        if (fireDist(m_gen) == 2) {
            std::cout << "!!!!!!!!!!!!!!shoot\n\n\n";
            sendShoot();
        }

        const bool ok = inspectTerritory();
        m_ai.clearVisible();
        return ok;
    }

    bool inspectTerritory()
    {
        if (canMove(m_dir)) {
            std::uniform_int_distribution<> changeDirDist(1, 20);
            if (changeDirDist(m_gen) != 1) {
                return sendMove();
            }
        }

        auto step = m_ai.randomStep();
        if (step) {
            auto want = desiredDirTo({m_ai.self.x + step->x, m_ai.self.y + step->y});
            if (want) {
                if (m_dir != *want) {
                    return sendRotate(*want);
                }
                return sendMove();
            }
        }

        static constexpr int8_t nextDir[4] = {3, 2, 1, 0};
        return sendRotate(nextDir[m_dir & 3]);
    }

    bool canMove(int8_t dir)
    {
        AiPos next = m_ai.self;
        if (dir == 0) next.x--;
        else if (dir == 1) next.x++;
        else if (dir == 2) next.y++;
        else if (dir == 3) next.y--;
        return !m_ai.blocked(next);
    }

    std::optional<int8_t> desiredDirTo(AiPos target) const
    {
        const int dx = target.x - m_ai.self.x;
        const int dy = target.y - m_ai.self.y;

        if (dx == 0 && dy == 0) {
            return std::nullopt;
        }

        if (aiAbs(dx) >= aiAbs(dy)) {
            if (dx < 0) return 0;
            if (dx > 0) return 1;
        }

        if (dy > 0) return 2;
        if (dy < 0) return 3;

        return std::nullopt;
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