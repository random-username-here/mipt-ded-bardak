#include "client-core/base/client_base.hpp"
#include "ghost_proto.hpp"
#include "srv_proto.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

class GhostClient : public ClientBase
{
  private:
	struct Pos
	{
		int32_t x = 0;
		int32_t y = 0;

		friend bool operator==(Pos lhs, Pos rhs)
		{
			return lhs.x == rhs.x && lhs.y == rhs.y;
		}

		struct Hash
		{
			std::size_t operator()(Pos pos) const
			{
				return std::hash<uint64_t>{}(
				    (static_cast<uint64_t>(static_cast<uint32_t>(pos.x)) << 32) |
				    static_cast<uint32_t>(pos.y));
			}
		};
	};

	struct Seen
	{
		Pos pos;
		uint32_t id = 0;
	};

	struct Bounds
	{
		int32_t min_x = 0;
		int32_t max_x = 0;
		int32_t min_y = 0;
		int32_t max_y = 0;
	};

	std::mt19937 m_rng{std::random_device{}()};

	bool m_alive = true;
	bool m_have_pos = false;
	int32_t m_hp = 0;
	Pos m_pos{};
	std::vector<Seen> m_visible;
	std::unordered_set<Pos, Pos::Hash> m_walls;

  public:
	GhostClient(const std::string &ini) : ClientBase(ini)
	{
		registerOnPrefix("ghost",
		                 [this](const PanFrame &frame) { return handleGhostFrame(frame); });
	}

  private:
	std::string_view roleName() const override
	{
		return "ghost";
	}

	bool keepRunning() const override
	{
		return m_alive;
	}

	bool handleGhostFrame(const PanFrame &frame)
	{
		const std::string raw = frame.rawMessage();
		bmsg::RawMessage msg(raw);
		const std::string_view type = frame.type();

		if (type == "tick") {
			if (!bmsg::SV_ghost_tick::decode(msg)) {
				return false;
			}
			return act();
		}
		if (type == "hp") {
			const auto hp = bmsg::SV_ghost_hp::decode(msg);
			if (!hp) {
				return false;
			}
			m_hp = hp->val;
			m_alive = m_hp > 0;
			return true;
		}
		if (type == "at") {
			const auto at = bmsg::SV_ghost_at::decode(msg);
			if (!at) {
				return false;
			}
			m_pos = {at->x, at->y};
			m_have_pos = true;
			m_walls.clear();
			return true;
		}
		if (type == "sees") {
			const auto sees = bmsg::SV_ghost_sees::decode(msg);
			if (!sees) {
				return false;
			}
			rememberTarget({sees->x, sees->y}, sees->who);
			return true;
		}
		if (type == "wall") {
			const auto wall = bmsg::SV_ghost_wall::decode(msg);
			if (!wall) {
				return false;
			}
			m_walls.insert({wall->x, wall->y});
			return true;
		}
		return true;
	}

	void rememberTarget(Pos pos, uint32_t id)
	{
		const auto it = std::find_if(m_visible.begin(), m_visible.end(),
		                             [id](const Seen &seen) { return seen.id == id; });

		if (it != m_visible.end()) {
			it->pos = pos;
			return;
		}
		m_visible.push_back({pos, id});
	}

	bool act()
	{
		if (!m_alive) {
			return true;
		}

		if (m_visible.empty()) {
			return randomMove();
		}

		const Seen target = nearestVisible();
		m_visible.clear();

		if (isAdjacent(target.pos)) {
			if (!sendMessage(bmsg::CL_ghost_attack{target.id})) {
				std::cerr << "send attack failed\n";
				m_alive = false;
				return false;
			}
			return true;
		}

		return moveToward(target.pos);
	}

	Seen nearestVisible() const
	{
		return *std::min_element(m_visible.begin(), m_visible.end(),
		                         [this](const Seen &lhs, const Seen &rhs) {
			                         return manhattan(lhs.pos) < manhattan(rhs.pos);
		                         });
	}

	int32_t manhattan(Pos pos) const
	{
		return std::abs(pos.x - m_pos.x) + std::abs(pos.y - m_pos.y);
	}

	bool isAdjacent(Pos pos) const
	{
		return std::abs(pos.x - m_pos.x) <= 1 && std::abs(pos.y - m_pos.y) <= 1;
	}

	bool knownWall(Pos pos) const
	{
		return m_walls.count(pos) != 0;
	}

	bool knownBounds(Bounds &bounds) const
	{
		if (m_walls.empty()) {
			return false;
		}

		bounds.min_x = bounds.max_x = m_walls.begin()->x;
		bounds.min_y = bounds.max_y = m_walls.begin()->y;
		for (const Pos wall : m_walls) {
			bounds.min_x = std::min(bounds.min_x, wall.x);
			bounds.max_x = std::max(bounds.max_x, wall.x);
			bounds.min_y = std::min(bounds.min_y, wall.y);
			bounds.max_y = std::max(bounds.max_y, wall.y);
		}
		return true;
	}

	static bool inBounds(Pos pos, Bounds bounds)
	{
		return pos.x >= bounds.min_x && pos.x <= bounds.max_x &&
		       pos.y >= bounds.min_y && pos.y <= bounds.max_y;
	}

	bool legalMove(Pos dir) const
	{
		if (!m_have_pos) {
			return true;
		}

		const Pos next{m_pos.x + dir.x, m_pos.y + dir.y};
		Bounds bounds;
		if (knownBounds(bounds) && !inBounds(next, bounds)) {
			return false;
		}
		return !knownWall(next);
	}

	std::vector<Pos> legalDirections()
	{
		std::array<Pos, 4> dirs{{
		    {1, 0},
		    {-1, 0},
		    {0, 1},
		    {0, -1},
		}};
		std::shuffle(dirs.begin(), dirs.end(), m_rng);

		std::vector<Pos> result;
		for (const Pos dir : dirs) {
			if (legalMove(dir)) {
				result.push_back(dir);
			}
		}
		return result;
	}

	bool sendMoveIfUseful(int8_t dx, int8_t dy)
	{
		if (dx == 0 && dy == 0) {
			return false;
		}

		if (!legalMove({dx, dy})) {
			return false;
		}

		if (!sendMessage(bmsg::CL_ghost_move{dx, dy})) {
			std::cerr << "send move failed\n";
			m_alive = false;
		}
		return true;
	}

	bool moveToward(Pos target)
	{
		const std::vector<Pos> moves = sortedMovesToward(target);
		if (moves.empty()) {
			return true;
		}

		const Pos move = moves.front();
		sendMoveIfUseful(static_cast<int8_t>(move.x), static_cast<int8_t>(move.y));
		return m_alive;
	}

	std::vector<Pos> sortedMovesToward(Pos target)
	{
		std::vector<Pos> moves = legalDirections();
		std::stable_sort(
		    moves.begin(),
		    moves.end(),
		    [this, target](Pos lhs, Pos rhs) {
			    const Pos lhs_next{m_pos.x + lhs.x, m_pos.y + lhs.y};
			    const Pos rhs_next{m_pos.x + rhs.x, m_pos.y + rhs.y};
			    return manhattanFrom(lhs_next, target) < manhattanFrom(rhs_next, target);
		    });
		return moves;
	}

	static int32_t manhattanFrom(Pos from, Pos to)
	{
		return std::abs(from.x - to.x) + std::abs(from.y - to.y);
	}

	bool randomMove()
	{
		const std::vector<Pos> legal_moves = legalDirections();
		if (legal_moves.empty()) {
			return true;
		}

		const Pos move = legal_moves.front();
		sendMoveIfUseful(static_cast<int8_t>(move.x), static_cast<int8_t>(move.y));
		return m_alive;
	}
	};

int main(int argc, char **argv)
{
	if (argc != 2) {
		std::cerr << "usage: " << argv[0] << " INI\n";
		return 1;
	}

	try {
		return GhostClient(argv[1]).run() ? 0 : 1;
	}
	catch (const std::exception &err) {
		std::cerr << err.what() << '\n';
		return 1;
	}
}
