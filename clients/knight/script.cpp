#include "client-core/base/client_base.hpp"
#include "knight_proto.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <queue>
#include <random>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class KnightClient : public ClientBase {
private:
    struct Pos {
        int32_t x = 0;
        int32_t y = 0;

        friend bool operator==(Pos lhs, Pos rhs)
        {
            return lhs.x == rhs.x && lhs.y == rhs.y;
        }

        friend bool operator!=(Pos lhs, Pos rhs)
        {
            return !(lhs == rhs);
        }

        struct Hash {
            std::size_t operator()(Pos pos) const
            {
                return std::hash<uint64_t>{}(
                    (static_cast<uint64_t>(static_cast<uint32_t>(pos.x)) << 32) |
                    static_cast<uint32_t>(pos.y));
            }
        };
    };

    struct RootInfo {
        Pos pos{};
        uint32_t id = 0;
    };

    static constexpr int32_t kMapLowerBound = 0;
    static constexpr int32_t kMapUpperBoundExclusive = 64;

    std::mt19937 m_rng{std::random_device{}()};
    bool    m_alive   = true;
    bool    m_havePos = false;
    int32_t m_hp = 0;
    Pos     m_pos{};
    std::vector<RootInfo> m_roots;
    std::unordered_set<Pos, Pos::Hash> m_walls;

public:
    KnightClient(const std::string &ini)
        : ClientBase(ini)
    {
        registerOnPrefix("knight", [this](const PanFrame &frame) {
            return handleKnightFrame(frame);
        });
    }

private:
    std::string_view roleName() const override
    {
        return "knight";
    }

    bool keepRunning() const override
    {
        return m_alive;
    }

    bool handleKnightFrame(const PanFrame &frame)
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
            m_hp    = hp->val;
            m_alive = m_hp > 0;
            return m_alive;
        }
        if (type == "at") {
            const auto at = bmsg::SV_knight_at::decode(msg);
            if (!at) {
                return false;
            }
            m_pos     = {at->x, at->y};
            m_havePos = true;
            return true;
        }
        if (type == "root") {
            const auto root = bmsg::SV_knight_root::decode(msg);
            if (!root) {
                return false;
            }
            rememberRoot({root->x, root->y}, root->who);
            return true;
        }
        if (type == "wall") {
            const auto wall = bmsg::SV_knight_wall::decode(msg);
            if (!wall) {
                return false;
            }
            m_walls.insert({wall->x, wall->y});
            return true;
        }
        return true;
    }

    void rememberRoot(Pos pos, uint32_t id)
    {
        const auto it = std::find_if(m_roots.begin(), m_roots.end(), [id](const RootInfo &root) {
            return root.id == id;
        });
        if (it != m_roots.end()) {
            it->pos = pos;
            return;
        }
        m_roots.push_back({pos, id});
    }

    bool actOnTick()
    {
        if (!m_alive) {
            return false;
        }
        if (!m_havePos) {
            clearTickRoots();
            return true;
        }

        if (attackAdjacentRoot()) {
            clearTickRoots();
            return m_alive;
        }

        if (moveTowardNearestRoot()) {
            clearTickRoots();
            return m_alive;
        }

        const bool ok = randomLegalMove();
        clearTickRoots();
        return ok;
    }

    bool attackAdjacentRoot()
    {
        const auto it = std::find_if(m_roots.begin(), m_roots.end(), [this](const RootInfo &root) {
            return manhattan(m_pos, root.pos) == 1;
        });

        if (it == m_roots.end()) {
            return false;
        }

        if (!sendMessage(bmsg::CL_knight_attack{it->id})) {
            std::cerr << "send attack failed\n";
            m_alive = false;
        }
        return true;
    }

    bool moveTowardNearestRoot()
    {
        if (m_roots.empty()) {
            return false;
        }

        const auto step = firstStepToAnyRootNeighbor();
        if (!step.has_value()) {
            return false;
        }

        return sendMoveIfLegal(static_cast<int8_t>(step->x), static_cast<int8_t>(step->y));
    }

    std::optional<Pos> firstStepToAnyRootNeighbor() const
    {
        static constexpr std::array<Pos, 4> dirs{{
            { 1,  0},
            {-1,  0},
            { 0,  1},
            { 0, -1},
        }};

        std::queue<Pos> queue;
        std::unordered_set<Pos,      Pos::Hash> visited;
        std::unordered_map<Pos, Pos, Pos::Hash> parent;

        queue  .push  (m_pos);
        visited.insert(m_pos);

        while (!queue.empty()) {
            const Pos cur = queue.front();
            queue.pop();

            if (cur != m_pos && isAdjacentToAnyRoot(cur)) {
                return reconstructFirstStep(parent, cur);
            }

            for (const Pos dir : dirs) {
                const Pos next{cur.x + dir.x, cur.y + dir.y};
                if (!inSearchBounds(next) || blocked(next) || visited.count(next) != 0) {
                    continue;
                }
                visited.insert (next);
                parent .emplace(next, cur);
                queue  .push   (next);
            }
        }

        return std::nullopt;
    }

    std::optional<Pos> reconstructFirstStep(
        const std::unordered_map<Pos, Pos, Pos::Hash> &parent,
        Pos target) const
    {
        Pos cur = target;
        while (true) {
            const auto it = parent.find(cur);
            if (it == parent.end()) {
                return std::nullopt;
            }
            if (it->second == m_pos) {
                return Pos{cur.x - m_pos.x, cur.y - m_pos.y};
            }
            cur = it->second;
        }
    }

    bool isAdjacentToAnyRoot(Pos pos) const
    {
        return std::any_of(m_roots.begin(), m_roots.end(), [pos](const RootInfo &root) {
            return manhattan(pos, root.pos) == 1;
        });
    }

    bool randomLegalMove()
    {
        static constexpr std::array<Pos, 4> dirs{{
            { 1,  0},
            {-1,  0},
            { 0,  1},
            { 0, -1},
        }};

        std::vector<Pos> legal;
        for (const Pos dir : dirs) {
            const Pos next{m_pos.x + dir.x, m_pos.y + dir.y};
            if (inSearchBounds(next) && !blocked(next)) {
                legal.push_back(dir);
            }
        }

        if (legal.empty()) {
            return true;
        }

        std::uniform_int_distribution<std::size_t> dist(0, legal.size() - 1);
        const Pos move = legal[dist(m_rng)];
        return sendMoveIfLegal(static_cast<int8_t>(move.x), static_cast<int8_t>(move.y));
    }

    bool sendMoveIfLegal(int8_t dx, int8_t dy)
    {
        if (dx == 0 && dy == 0) {
            return false;
        }

        const Pos next{m_pos.x + dx, m_pos.y + dy};
        if (blocked(next)) {
            return false;
        }

        if (!sendMessage(bmsg::CL_knight_move{dx, dy})) {
            std::cerr << "send move failed\n";
            m_alive = false;
            return false;
        }
        return true;
    }

    bool blocked(Pos pos) const
    {
        if (m_walls.count(pos) != 0) {
            return true;
        }
        return std::any_of(m_roots.begin(), m_roots.end(), [pos](const RootInfo &root) {
            return root.pos == pos;
        });
    }

    static bool inSearchBounds(Pos pos)
    {
        return pos.x >= kMapLowerBound && pos.y >= kMapLowerBound &&
               pos.x < kMapUpperBoundExclusive && pos.y < kMapUpperBoundExclusive;
    }

    static int32_t manhattan(Pos lhs, Pos rhs)
    {
        return std::abs(lhs.x - rhs.x) + std::abs(lhs.y - rhs.y);
    }

    void clearTickRoots()
    {
        m_roots.clear();
    }
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " INI\n";
        return 1;
    }

    try {
        return KnightClient(argv[1]).run() ? 0 : 1;
    }
    catch (const std::exception &err) {
        std::cerr << err.what() << '\n';
        return 1;
    }
}
