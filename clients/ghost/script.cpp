#include "../utils/tcp_connection.hpp"
#include "../../mods/person/ghost/ghost_proto.hpp"
#include "../../mods/role_manager/role_proto.hpp"
#include "../../servers/msva/src/srv_proto.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <unistd.h>
#include <vector>

class GhostClient {
private:
	struct Pos {
		int32_t x;
		int32_t y;

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
		uint32_t id;
	};

	TcpConnection m_conn;
	bool m_alive = true;
	bool m_have_pos = false;
	int32_t m_hp = 0;
	Pos m_pos { 0, 0 };
	std::vector<Seen> m_visible;
	std::unordered_set<Pos, Pos::Hash> m_walls;

public:
	GhostClient(const char *host, const char *port)
		: m_conn(host, port)
	{}

	int run()
	{
		if (chooseGhostRole() != 0) {
			fprintf(stderr, "ghost role choice error\n");
			return 1;
		}

		return waitForMessages([this](PanHeader *hdr, const uint8_t *payload, uint16_t len) {
			if (strcmp(hdr->prefix, "ghost") == 0) {
				return handleGhostMsg(hdr, payload, len, true);
			}
			if (strcmp(hdr->prefix, "srv") == 0) {
				handleSrvMsg(hdr, payload, len);
				return 0;
			}
			return 0;
		});
	}

private:
	template <typename Callback>
	int waitForMessages(Callback cb)
	{
		uint8_t payload[PAN_MAX_PAYLOAD];

		while (m_alive) {
			PanHeader hdr;
			int rc = m_conn.readHeader(&hdr);
			if (rc == 1) {
				continue;
			}
			if (rc == 2) {
				fprintf(stderr, "server closed connection\n");
				break;
			}
			if (rc != 0) {
				perror("read header");
				break;
			}

			const uint16_t len = hdr.len;
			if (len > 0) {
				rc = m_conn.readExact(payload, len);
				if (rc == 1) {
					fprintf(stderr, "payload timeout\n");
					break;
				}
				if (rc == 2) {
					fprintf(stderr, "server closed during payload read\n");
					break;
				}
				if (rc != 0) {
					perror("read payload");
					break;
				}
			}

			if (cb(&hdr, payload, len) != 0) {
				return 1;
			}
		}

		return 0;
	}

	std::string makeRawMessage(const PanHeader &hdr, const uint8_t *payload)
	{
		std::string raw;
		raw.resize(PAN_HEADER_LEN + hdr.len);
		fill_name8(reinterpret_cast<uint8_t *>(raw.data()) + 0, hdr.prefix);
		fill_name8(reinterpret_cast<uint8_t *>(raw.data()) + 8, hdr.type);
		wr_u32_le(reinterpret_cast<uint8_t *>(raw.data()) + 16, hdr.id);
		wr_u16_le(reinterpret_cast<uint8_t *>(raw.data()) + 20, hdr.len);
		wr_u16_le(reinterpret_cast<uint8_t *>(raw.data()) + 22, hdr.flags);
		if (hdr.len > 0) {
			memcpy(raw.data() + PAN_HEADER_LEN, payload, hdr.len);
		}
		return raw;
	}

	template <typename Msg>
	int sendMessage(const Msg &msg)
	{
		std::ostringstream os;
		msg.encode(os, 0, 0);
		return m_conn.writeRaw(os.str());
	}

	int chooseGhostRole()
	{
		if (sendMessage(bmsg::CL_role_choose { "ghost" }) != 0) {
			fprintf(stderr, "send ghost role failed\n");
			return 1;
		}

		bool role_chosen = false;
		return waitForMessages([this, &role_chosen](PanHeader *hdr, const uint8_t *payload, uint16_t len) {
			if (strcmp(hdr->prefix, "role") == 0) {
				const int rc = handleRoleMsg(hdr, payload);
				if (rc == 0 && strcmp(hdr->type, "chosen") == 0) {
					role_chosen = true;
					m_alive = false;
				}
				return rc;
			}
			if (strcmp(hdr->prefix, "ghost") == 0) {
				return handleGhostMsg(hdr, payload, len, false);
			}
			if (strcmp(hdr->prefix, "srv") == 0) {
				handleSrvMsg(hdr, payload, len);
			}
			return 0;
		}) == 0 && role_chosen ? (m_alive = true, 0) : 1;
	}

	int handleRoleMsg(PanHeader *hdr, const uint8_t *payload)
	{
		const std::string raw = makeRawMessage(*hdr, payload);
		bmsg::RawMessage msg(raw);

		if (strcmp(hdr->type, "chosen") == 0) {
			return bmsg::SV_role_chosen::decode(msg) ? 0 : 1;
		}
		if (strcmp(hdr->type, "option") == 0) {
			return bmsg::SV_role_option::decode(msg) ? 0 : 1;
		}
		if (strcmp(hdr->type, "reject") == 0) {
			const auto reject = bmsg::SV_role_reject::decode(msg);
			if (reject) {
				fprintf(stderr, "role choice reject, reason: %.*s\n", (int)reject->reason.size(), reject->reason.data());
			} else {
				fprintf(stderr, "role choice reject\n");
			}
			return 1;
		}
		return 0;
	}

	int handleGhostMsg(PanHeader *hdr, const uint8_t *payload, uint16_t, bool act_on_tick)
	{
		const std::string raw = makeRawMessage(*hdr, payload);
		bmsg::RawMessage msg(raw);

		if (strcmp(hdr->type, "tick") == 0) {
			if (!bmsg::SV_ghost_tick::decode(msg)) {
				return 0;
			}
			if (act_on_tick) {
				act();
			}
			return 0;
		}
		if (strcmp(hdr->type, "hp") == 0) {
			const auto hp = bmsg::SV_ghost_hp::decode(msg);
			if (!hp) {
				return 0;
			}
			m_hp = hp->val;
			fprintf(stderr, "hp = %d\n", m_hp);
			if (m_hp <= 0) {
				fprintf(stderr, "dead; closing connection\n");
				m_alive = false;
			}
			return 0;
		}
		if (strcmp(hdr->type, "at") == 0) {
			const auto at = bmsg::SV_ghost_at::decode(msg);
			if (!at) {
				return 0;
			}
			m_pos = { at->x, at->y };
			m_have_pos = true;
			fprintf(stderr, "at = (%d, %d)\n", m_pos.x, m_pos.y);
			return 0;
		}
		if (strcmp(hdr->type, "sees") == 0) {
			const auto sees = bmsg::SV_ghost_sees::decode(msg);
			if (!sees) {
				return 0;
			}
			rememberVisible({ sees->x, sees->y }, sees->who);
			fprintf(stderr, "sees id=%u at (%d, %d)\n", sees->who, sees->x, sees->y);
			return 0;
		}
		if (strcmp(hdr->type, "wall") == 0) {
			const auto wall = bmsg::SV_ghost_wall::decode(msg);
			if (!wall) {
				return 0;
			}
			m_walls.insert({ wall->x, wall->y });
			fprintf(stderr, "wall at (%d, %d)\n", wall->x, wall->y);
			return 0;
		}
		return 0;
	}

	void handleSrvMsg(PanHeader *hdr, const uint8_t *payload, uint16_t)
	{
		const std::string raw = makeRawMessage(*hdr, payload);
		bmsg::RawMessage msg(raw);

		if (strcmp(hdr->type, "hasPref") == 0) {
			const auto has_pref = bmsg::SV_srv_hasPref::decode(msg);
			if (!has_pref) {
				return;
			}
			bmsg::Char64 pref_value = has_pref->pref;
			const std::string_view pref = pref_value;
			fprintf(stderr, "srv:hasPref pref='%.*s'\n", (int)pref.size(), pref.data());
			return;
		}
		if (strcmp(hdr->type, "name") == 0) {
			const auto name = bmsg::SV_srv_name::decode(msg);
			if (name) {
				fprintf(stderr, "srv:name name='%.*s'\n", (int)name->name.size(), name->name.data());
			}
			return;
		}
		if (strcmp(hdr->type, "id") == 0) {
			const auto id = bmsg::SV_srv_id::decode(msg);
			if (id) {
				fprintf(stderr, "srv:id clientId=%u\n", id->clientId);
			}
			return;
		}
		if (strcmp(hdr->type, "level") == 0) {
			const auto level = bmsg::SV_srv_level::decode(msg);
			if (!level) {
				return;
			}
			bmsg::Char64 level_value = level->level;
			const std::string_view level_view = level_value;
			fprintf(stderr, "srv:level level='%.*s'\n", (int)level_view.size(), level_view.data());
		}
	}

	void rememberVisible(Pos pos, uint32_t id)
	{
		for (auto &seen : m_visible) {
			if (seen.id == id) {
				seen.pos = pos;
				return;
			}
		}
		m_visible.push_back({ pos, id });
	}

	void act()
	{
		if (!m_alive) {
			return;
		}

		if (!m_visible.empty()) {
			const Seen target = nearestVisible();
			if (isAdjacent(target.pos)) {
				if (sendMessage(bmsg::CL_ghost_attack { target.id }) != 0) {
					fprintf(stderr, "send attack failed\n");
					m_alive = false;
				} else {
					fprintf(stderr, "attack %u\n", target.id);
				}
			} else {
				moveToward(target.pos);
			}
			m_visible.clear();
			return;
		}

		randomMove();
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

	bool knownWall(int32_t x, int32_t y) const
	{
		return m_walls.count({ x, y }) != 0;
	}

	bool sendMoveIfUseful(int8_t dx, int8_t dy)
	{
		if (dx == 0 && dy == 0) {
			return false;
		}
		if (m_have_pos && knownWall(m_pos.x + dx, m_pos.y + dy)) {
			return false;
		}
		if (sendMessage(bmsg::CL_ghost_move { dx, dy }) != 0) {
			fprintf(stderr, "send move failed\n");
			m_alive = false;
			return true;
		}
		fprintf(stderr, "move %d %d\n", (int)dx, (int)dy);
		return true;
	}

	void moveToward(Pos target)
	{
		const int8_t dx = sign(target.x - m_pos.x);
		const int8_t dy = sign(target.y - m_pos.y);

		if (sendMoveIfUseful(dx, dy)) {
			return;
		}
		if (sendMoveIfUseful(dx, 0)) {
			return;
		}
		if (sendMoveIfUseful(0, dy)) {
			return;
		}

		randomMove();
	}

	void randomMove()
	{
		static const int8_t dirs[4][2] = {
			{  1,  0 },
			{ -1,  0 },
			{  0,  1 },
			{  0, -1 },
		};

		std::vector<Pos> legal;
		for (const auto &dir : dirs) {
			const Pos next { m_pos.x + dir[0], m_pos.y + dir[1] };
			if (!m_have_pos || !knownWall(next.x, next.y)) {
				legal.push_back({ dir[0], dir[1] });
			}
		}

		if (legal.empty()) {
			fprintf(stderr, "boxed in by known walls; skipping tick\n");
			return;
		}

		const Pos move = legal[(size_t)(rand() % (int)legal.size())];
		sendMoveIfUseful((int8_t)move.x, (int8_t)move.y);
	}
};

int main(int argc, char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "usage: %s HOST PORT\n", argv[0]);
		return 1;
	}

	srand((unsigned int)(time(nullptr) ^ (unsigned int)getpid()));

	try {
		return GhostClient(argv[1], argv[2]).run();
	} catch (const std::exception &err) {
		fprintf(stderr, "%s\n", err.what());
		return 1;
	}
}
