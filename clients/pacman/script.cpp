#include "../utils/client_base.hpp"
#include "../../mods/person/pacman/pacman_proto.hpp"
#include "../../servers/msva/src/srv_proto.hpp"

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

class Pacman : public ClientBase
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

	std::mt19937 m_rng{std::random_device{}()};

	bool m_alive = true;
	bool m_have_pos = false;
	int32_t m_hp = 0;
	Pos m_pos{};
	std::unordered_set<uint32_t> m_visible_ids;
	std::unordered_set<Pos, Pos::Hash> m_walls;

  public:
	Pacman(const std::string &ini) : ClientBase(ini)
	{
		registerOnPrefix("pacman",
		                 [this](const PanFrame &frame) { return handlePacmanMsg(frame); });
		registerOnPrefix("srv", [this](const PanFrame &frame) { return handleSrvMsg(frame); });
	}

  private:
	std::string_view roleName() const override
	{
		return "pacman";
	}

	bool keepRunning() const override
	{
		return m_alive;
	}

	bool handlePacmanMsg(const PanFrame &frame)
	{
		const std::string raw = frame.rawMessage();
		bmsg::RawMessage msg(raw);
		const std::string_view type = frame.type();

		if (type == "tick") {
			return bmsg::SV_pacman_tick::decode(msg) ? tick() : false;
		}
		if (type == "hp") {
			const auto hp = bmsg::SV_pacman_hp::decode(msg);
			if (!hp) {
				return false;
			}
			m_hp = hp->val;
			m_alive = m_hp > 0;
			return true;
		}
		if (type == "at") {
			const auto at = bmsg::SV_pacman_at::decode(msg);
			if (!at) {
				return false;
			}
			m_pos = {at->x, at->y};
			m_have_pos = true;
			return true;
		}
		if (type == "sees") {
			const auto sees = bmsg::SV_pacman_sees::decode(msg);
			if (!sees) {
				return false;
			}
			m_visible_ids.insert(sees->who);
			return true;
		}
		if (type == "wall") {
			const auto wall = bmsg::SV_pacman_wall::decode(msg);
			if (!wall) {
				return false;
			}
			m_walls.insert({wall->x, wall->y});
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

	bool tick()
	{
		if (!m_alive) {
			return true;
		}

		static constexpr std::array<Pos, 4> dirs{{
		    {1, 0},
		    {-1, 0},
		    {0, 1},
		    {0, -1},
		}};

		std::vector<Pos> legal_moves;
		for (const Pos dir : dirs) {
			const Pos next{m_pos.x + dir.x, m_pos.y + dir.y};
			if (!m_have_pos || m_walls.count(next) == 0) {
				legal_moves.push_back(dir);
			}
		}

		if (legal_moves.empty()) {
			return true;
		}

		std::uniform_int_distribution<std::size_t> dist(0, legal_moves.size() - 1);
		const Pos move = legal_moves[dist(m_rng)];
		if (!sendMove(static_cast<int8_t>(move.x), static_cast<int8_t>(move.y))) {
			std::cerr << "send move failed\n";
			m_alive = false;
			return false;
		}
		return true;
	}

	bool sendMove(int8_t dx, int8_t dy)
	{
		return sendMessage(bmsg::CL_pacman_move{dx, dy});
	}
};

int main(int argc, char **argv)
{
	if (argc != 2) {
		std::cerr << "usage: " << argv[0] << " INI\n";
		return 1;
	}

	try {
		return Pacman(argv[1]).run() ? 0 : 1;
	}
	catch (const std::exception &err) {
		std::cerr << err.what() << '\n';
		return 1;
	}
}
