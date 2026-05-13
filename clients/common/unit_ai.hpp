#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <queue>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct AiPos {
    int32_t x = 0;
    int32_t y = 0;

    friend bool operator==(AiPos lhs, AiPos rhs)
    {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }

    friend bool operator!=(AiPos lhs, AiPos rhs)
    {
        return !(lhs == rhs);
    }

    struct Hash {
        std::size_t operator()(AiPos pos) const
        {
            return std::hash<uint64_t>{}(
                (static_cast<uint64_t>(static_cast<uint32_t>(pos.x)) << 32) |
                 static_cast<uint32_t>(pos.y)
            );
        }
    };
};

struct AiSeenEntity {
    AiPos pos{};
    uint32_t id = 0;
    std::string kind;
};

enum class AiRangeKind {
    Moore,
    VonNeumann,
    ArcherCircle,
    MageFlameStar,
};

inline int aiAbs(int32_t v)
{
    return v < 0 ? -v : v;
}

inline int32_t aiManhattan(AiPos lhs, AiPos rhs)
{
    return aiAbs(lhs.x - rhs.x) + aiAbs(lhs.y - rhs.y);
}

inline bool aiInRange(AiPos from, AiPos to, AiRangeKind range)
{
    const int dx = aiAbs(from.x - to.x);
    const int dy = aiAbs(from.y - to.y);

    if (dx == 0 && dy == 0) {
        return false;
    }

    if (range == AiRangeKind::Moore) {
        return dx <= 1 && dy <= 1;
    }

    if (range == AiRangeKind::ArcherCircle) {
        return dx <= 2 && dy <= 2 && !(dx == 2 && dy == 2);
    }

    if (range == AiRangeKind::MageFlameStar) {
        return dx + dy <= 2;
    }

    return dx + dy == 1;
}

class UnitAi {
    static constexpr int32_t kMapLowerBound          = 0;
    static constexpr int32_t kMapUpperBoundExclusive = 64;

    AiRangeKind  m_range = AiRangeKind::VonNeumann;
    std::mt19937 m_rng{std::random_device{}()};

public:
    AiPos self{};
    bool haveSelf = false;

    std::vector<AiSeenEntity> roots;
    std::vector<AiSeenEntity> enemies;
    std::unordered_set<AiPos, AiPos::Hash> walls;

    explicit UnitAi(AiRangeKind range)
        : m_range(range)
    {}

    bool inAttackRange(AiPos target) const
    {
        return aiInRange(self, target, m_range);
    }

    std::optional<AiSeenEntity> firstEnemyInRange() const
    {
        return firstInRange(enemies);
    }

    std::optional<AiSeenEntity> firstRootInRange() const
    {
        return firstInRange(roots);
    }

    std::optional<AiPos> stepTowardEnemyRange() const
    {
        return stepTowardRangeOf(enemies);
    }

    std::optional<AiPos> stepTowardRootRange() const
    {
        return stepTowardRangeOf(roots);
    }

    std::optional<AiPos> randomStep()
    {
        static constexpr std::array<AiPos, 4> dirs{{
            { 1,  0},
            {-1,  0},
            { 0,  1},
            { 0, -1},
        }};

        std::vector<AiPos> legal;

        for (const AiPos dir : dirs) {
            const AiPos next{self.x + dir.x, self.y + dir.y};
            if (inSearchBounds(next) && !blocked(next)) {
                legal.push_back(dir);
            }
        }

        if (legal.empty()) {
            return std::nullopt;
        }

        std::uniform_int_distribution<std::size_t> dist(0, legal.size() - 1);
        return legal[dist(m_rng)];
    }

    bool blocked(AiPos pos) const
    {
        if (walls.count(pos) != 0) {
            return true;
        }

        for (const auto &root : roots) {
            if (root.pos == pos) {
                return true;
            }
        }

        for (const auto &enemy : enemies) {
            if (enemy.pos == pos) {
                return true;
            }
        }

        return false;
    }

    void clearVisible()
    {
        roots  .clear();
        enemies.clear();
        walls  .clear();
    }

private:
    std::optional<AiSeenEntity> firstInRange(const std::vector<AiSeenEntity> &entities) const
    {
        const auto it = std::find_if(entities.begin(), entities.end(), [this](const AiSeenEntity &entity) {
            return inAttackRange(entity.pos);
        });

        if (it == entities.end()) {
            return std::nullopt;
        }

        return *it;
    }

    std::optional<AiPos> stepTowardRangeOf(const std::vector<AiSeenEntity> &targets) const
    {
        if (targets.empty() || !haveSelf) {
            return std::nullopt;
        }

        static constexpr std::array<AiPos, 4> dirs{{
            {1, 0},
            {-1, 0},
            {0, 1},
            {0, -1},
        }};

        std::queue        <AiPos>                     queue;
        std::unordered_set<AiPos,        AiPos::Hash> visited;
        std::unordered_map<AiPos, AiPos, AiPos::Hash> parent;

        queue  .push  (self);
        visited.insert(self);

        while (!queue.empty()) {
            const AiPos cur = queue.front();
            queue.pop();

            if (cur != self && attacksAny(cur, targets) && !blocked(cur)) {
                return reconstructFirstStep(parent, cur);
            }

            for (const AiPos dir : dirs) {
                const AiPos next{cur.x + dir.x, cur.y + dir.y};

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

    bool attacksAny(AiPos pos, const std::vector<AiSeenEntity> &targets) const
    {
        return std::any_of(targets.begin(), targets.end(), [&](const AiSeenEntity &target) {
            return aiInRange(pos, target.pos, m_range);
        });
    }

    std::optional<AiPos> reconstructFirstStep(
        const std::unordered_map<AiPos, AiPos, AiPos::Hash> &parent,
        AiPos target) const
    {
        AiPos cur = target;

        while (true) {
            const auto it = parent.find(cur);
            if (it == parent.end()) {
                return std::nullopt;
            }

            if (it->second == self) {
                return AiPos{cur.x - self.x, cur.y - self.y};
            }

            cur = it->second;
        }
    }

    static bool inSearchBounds(AiPos pos)
    {
        return pos.x >= kMapLowerBound && pos.y >= kMapLowerBound &&
               pos.x < kMapUpperBoundExclusive && pos.y < kMapUpperBoundExclusive;
    }
};
