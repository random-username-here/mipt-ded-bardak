#include "../utils/client_base.h"
#include "../../mods/person/ghost/ghost_proto.hpp"
#include "../../servers/msva/src/srv_proto.hpp"

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

class GhostClient : public ClientBase {
private:
	struct Pos {
		int32_t x = 0;
		int32_t y = 0;

		friend bool operator==(Pos lhs, Pos rhs)
		{
			return lhs.x == rhs.x && lhs.y == rhs.y;
		}

		struct Hash {
			std::size_t operator()(Pos pos) const
			{
				return std::hash<uint64_t>{}(
					(static_cast<uint64_t>(static_cast<uint32_t>(pos.x)) << 32) |
					static_cast<uint32_t>(pos.y)
				);
			}
		};
	};

	struct Seen {
		Pos pos;
		uint32_t id = 0;
	};

	std::mt19937 m_rng { std::random_device{}() };

	bool m_alive = true;
	bool m_have_pos = false;
	int32_t m_hp = 0;
	Pos m_pos {};
	std::vector<Seen> m_visible;
	std::unordered_set<Pos, Pos::Hash> m_walls;

public:
	GhostClient(std::string_view host, std::string_view port)
		: ClientBase(host, port)
	{
		registerOnPrefix("ghost", [this](const PanFrame &frame) {
			return handleGhostMsg(frame);
		});
		registerOnPrefix("srv", [this](const PanFrame &frame) {
			return handleSrvMsg(frame);
		});
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

	bool handleGhostMsg(const PanFrame &frame)
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
			m_pos = { at->x, at->y };
			m_have_pos = true;
			return true;
		}
		if (type == "sees") {
			const auto sees = bmsg::SV_ghost_sees::decode(msg);
			if (!sees) {
				return false;
			}
			rememberVisible({ sees->x, sees->y }, sees->who);
			return true;
		}
		if (type == "wall") {
			const auto wall = bmsg::SV_ghost_wall::decode(msg);
			if (!wall) {
				return false;
			}
			m_walls.insert({ wall->x, wall->y });
			return true;
		}
		return true;
	}

	bool handleSrvMsg(const PanFrame &frame)
	{
		const std::string raw = frame.rawMessage();
		bmsg::RawMessage msg(raw);
		const std::string_view type = frame.type();

		if (type == "hasPref") {
			return bmsg::SV_srv_hasPref::decode(msg).has_value();
		}
		if (type == "name") {
			return bmsg::SV_srv_name::decode(msg).has_value();
		}
		if (type == "id") {
			return bmsg::SV_srv_id::decode(msg).has_value();
		}
		if (type == "level") {
			return bmsg::SV_srv_level::decode(msg).has_value();
		}
		if (type == "r.setLvl") {
			return bmsg::SV_srv_r_setLvl::decode(msg).has_value();
		}
		return true;
	}

	void rememberVisible(Pos pos, uint32_t id)
	{
		const auto it = std::find_if(m_visible.begin(), m_visible.end(), [id](const Seen &seen) {
			return seen.id == id;
		});

		if (it != m_visible.end()) {
			it->pos = pos;
			return;
		}
		m_visible.push_back({ pos, id });
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
			if (!sendMessage(bmsg::CL_ghost_attack { target.id })) {
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
		return *std::min_element(m_visible.begin(), m_visible.end(), [this](const Seen &lhs, const Seen &rhs) {
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

	static int8_t sign(int32_t value)
	{
		if (value > 0) {
			return 1;
		}
		if (value < 0) {
			return -1;
		}
		return 0;
	}

	bool knownWall(Pos pos) const
	{
		return m_walls.count(pos) != 0;
	}

	bool sendMoveIfUseful(int8_t dx, int8_t dy)
	{
		if (dx == 0 && dy == 0) {
			return false;
		}

		const Pos next { m_pos.x + dx, m_pos.y + dy };
		if (m_have_pos && knownWall(next)) {
			return false;
		}

		if (!sendMessage(bmsg::CL_ghost_move { dx, dy })) {
			std::cerr << "send move failed\n";
			m_alive = false;
		}
		return true;
	}

	bool moveToward(Pos target)
	{
		const int8_t dx = sign(target.x - m_pos.x);
		const int8_t dy = sign(target.y - m_pos.y);

		if (sendMoveIfUseful(dx, dy)) {
			return m_alive;
		}
		if (sendMoveIfUseful(dx, 0)) {
			return m_alive;
		}
		if (sendMoveIfUseful(0, dy)) {
			return m_alive;
		}

		return randomMove();
	}

	bool randomMove()
	{
		static constexpr std::array<Pos, 4> dirs {{
			{  1,  0 },
			{ -1,  0 },
			{  0,  1 },
			{  0, -1 },
		}};

		std::vector<Pos> legal_moves;
		for (const Pos dir : dirs) {
			const Pos next { m_pos.x + dir.x, m_pos.y + dir.y };
			if (!m_have_pos || !knownWall(next)) {
				legal_moves.push_back(dir);
			}
		}

		if (legal_moves.empty()) {
			return true;
		}

		std::uniform_int_distribution<std::size_t> dist(0, legal_moves.size() - 1);
		const Pos move = legal_moves[dist(m_rng)];
		sendMoveIfUseful(static_cast<int8_t>(move.x), static_cast<int8_t>(move.y));
		return m_alive;
	}
};

int main(int argc, char **argv)
{
	if (argc != 3) {
		std::cerr << "usage: " << argv[0] << " HOST PORT\n";
		return 1;
	}

	try {
		return GhostClient(argv[1], argv[2]).run() ? 0 : 1;
	}
	catch (const std::exception &err) {
		std::cerr << err.what() << '\n';
		return 1;
	}
}
