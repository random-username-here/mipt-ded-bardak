#include "client-core/base/client_base.hpp"
#include "person_proto.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

class PersonClient : public ClientBase
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

	struct Visible
	{
		Pos pos{};
		uint32_t id = 0;
	};

	std::mt19937 m_rng{std::random_device{}()};

	bool m_alive = true;
	bool m_have_pos = false;
	int32_t m_hp = 0;
	Pos m_pos{};
	std::vector<Visible> m_visible;
	std::unordered_set<Pos, Pos::Hash> m_walls;

  public:
	PersonClient(const std::string &ini) : ClientBase(ini)
	{
		registerOnPrefix("person",
		                 [this](const PanFrame &frame) { return handlePersonFrame(frame); });
	}

  private:
	std::string_view roleName() const override
	{
		return "person";
	}

	bool keepRunning() const override
	{
		return m_alive;
	}

	static int32_t longDist(Pos a, Pos b)
	{
		const int32_t dx = std::abs(a.x - b.x);
		const int32_t dy = std::abs(a.y - b.y);
		return dx > dy ? dx : dy;
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

	void rememberVisible(Pos pos, uint32_t id)
	{
		const auto it = std::find_if(m_visible.begin(), m_visible.end(),
		                             [id](const Visible &v) { return v.id == id; });
		if (it != m_visible.end()) {
			it->pos = pos;
			return;
		}
		m_visible.push_back({pos, id});
	}

	void clearTickState()
	{
		m_visible.clear();
	}

	bool sendMoveIfUseful(int8_t dx, int8_t dy)
	{
		if (dx == 0 && dy == 0) {
			return false;
		}

		const Pos next{m_pos.x + dx, m_pos.y + dy};
		if (m_have_pos && knownWall(next)) {
			return false;
		}

		if (!sendMessage(bmsg::CL_person_move{dx, dy})) {
			std::cerr << "send move failed\n";
			m_alive = false;
		}
		return true;
	}

	bool tryChaseToward(Pos target)
	{
		const int8_t sx = sign(target.x - m_pos.x);
		const int8_t sy = sign(target.y - m_pos.y);

		// if (sendMoveIfUseful(sx, sy)) {
			// return m_alive;
		// }
		if (std::abs(target.x - m_pos.x) >= std::abs(target.y - m_pos.y)) {
			if (sendMoveIfUseful(sx, 0)) {
				return m_alive;
			}
			if (sendMoveIfUseful(0, sy)) {
				return m_alive;
			}
		} else {
			if (sendMoveIfUseful(0, sy)) {
				return m_alive;
			}
			if (sendMoveIfUseful(sx, 0)) {
				return m_alive;
			}
		}
		return false;
	}

	bool randomLegalMove()
	{
		static constexpr std::array<Pos, 4> dirs{{
		    {1, 0},
		    {-1, 0},
		    {0, 1},
		    {0, -1},
		}};

		std::vector<Pos> legal;
		for (const Pos dir : dirs) {
			const Pos next{m_pos.x + dir.x, m_pos.y + dir.y};
			if (!m_have_pos || !knownWall(next)) {
				legal.push_back(dir);
			}
		}

		if (legal.empty()) {
			std::cerr << "boxed in by known walls; skipping tick\n";
			return true;
		}

		std::uniform_int_distribution<std::size_t> dist(0, legal.size() - 1);
		const Pos move = legal[dist(m_rng)];
		sendMoveIfUseful(static_cast<int8_t>(move.x), static_cast<int8_t>(move.y));
		return m_alive;
	}

	bool actOnTick()
	{
		if (!m_alive) {
			return true;
		}

		std::vector<Visible> attackable;
		if (m_have_pos) {
			for (const Visible &v : m_visible) {
				const int32_t dx = std::abs(v.pos.x - m_pos.x);
				const int32_t dy = std::abs(v.pos.y - m_pos.y);
				if (dx <= 1 && dy <= 1) {
					attackable.push_back(v);
				}
			}
		}

		if (!attackable.empty()) {
			std::uniform_int_distribution<std::size_t> dist(0, attackable.size() - 1);
			const uint32_t target = attackable[dist(m_rng)].id;
			if (!sendMessage(bmsg::CL_person_attack{target})) {
				std::cerr << "send attack failed\n";
				m_alive = false;
				clearTickState();
				return false;
			}
			clearTickState();
			return true;
		}

		if (m_have_pos && !m_visible.empty()) {
			const Visible &nearest = *std::min_element(
			    m_visible.begin(), m_visible.end(),
			    [this](const Visible &lhs, const Visible &rhs) {
				    return longDist(m_pos, lhs.pos) < longDist(m_pos, rhs.pos);
			    });

			const Pos target = nearest.pos;
			if (tryChaseToward(target)) {
				clearTickState();
				return m_alive;
			}
		}

		const bool ok = randomLegalMove();
		clearTickState();
		return ok;
	}

	bool handlePersonFrame(const PanFrame &frame)
	{
		const std::string raw = frame.rawMessage();
		bmsg::RawMessage msg(raw);
		const std::string_view type = frame.type();

		if (type == "tick") {
			if (!bmsg::SV_person_tick::decode(msg)) {
				return false;
			}
			return actOnTick();
		}
		if (type == "hp") {
			const auto hp = bmsg::SV_person_hp::decode(msg);
			if (!hp) {
				return false;
			}
			m_hp = hp->val;
			m_alive = m_hp > 0;
			return true;
		}
		if (type == "at") {
			const auto at = bmsg::SV_person_at::decode(msg);
			if (!at) {
				return false;
			}
			m_pos = {at->x, at->y};
			m_have_pos = true;
			return true;
		}
		if (type == "sees") {
			const auto sees = bmsg::SV_person_sees::decode(msg);
			if (!sees) {
				return false;
			}
			rememberVisible({sees->x, sees->y}, sees->who);
			return true;
		}
		if (type == "wall") {
			const auto wall = bmsg::SV_person_wall::decode(msg);
			if (!wall) {
				return false;
			}
			m_walls.insert({wall->x, wall->y});
			return true;
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
		return PersonClient(argv[1]).run() ? 0 : 1;
	}
	catch (const std::exception &err) {
		std::cerr << err.what() << '\n';
		return 1;
	}
}
