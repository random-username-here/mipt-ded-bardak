#include "../utils/tcp_connection.h"
#include "../../mods/person/pacman/pacman_proto.hpp"
#include "../../mods/role_manager/role_proto.hpp"
#include "../../servers/msva/src/srv_proto.hpp"

#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <unordered_set>

class Pacman
{
  private:
	struct Pos
	{
		int x;
		int y;

		friend bool operator==(Pos lhs, Pos rhs)
		{
			return lhs.x == rhs.x && lhs.y == rhs.y;
		}

		struct Hash
		{
			std::size_t
			operator () (Pos pos) const
			{
				return std::hash<std::size_t>{}(
					( static_cast<uint64_t>(pos.x) << 32 ) |
					static_cast<uint64_t>(pos.y)
				);    
			}
		};
	};

	int m_fd = -1;

	bool m_alive = true;
	int m_have_pos = 0;

	int32_t m_hp = 0;
	Pos m_coord { 0, 0 };
	std::unordered_set<uint32_t> m_visible_ids;
	std::unordered_set<Pos, Pos::Hash> m_walls;

  public:
	int
	run(int fd)
	{
		m_fd = fd;

		uint8_t payload[PAN_MAX_PAYLOAD];

		if (choosePacmanRole() != 0)
		{
			fprintf(stderr, "pacman role choice error\n");
			return 1;
		}

		int main_loop_status = waitForAnswer(
			[this](){return m_alive;},
			[this](PanHeader *hdr, const uint8_t *payload, uint16_t len){
				if (strcmp(hdr->prefix, "pacman") == 0) {
					return handlePacmanMsg(hdr->type, payload, len);
				} else if (strcmp(hdr->prefix, "srv") == 0) {
					return handleSrvMsg(hdr->type, payload, len);
				}

				return 0;
			}
		);

		if (main_loop_status != 0)
		{
			fprintf(stderr, "mainloop error\n");
			return 1;
		}

		return 0;
	}

  private:
	std::string
	makeRawMessage(const PanHeader &hdr, const uint8_t *payload)
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
	int
	sendMessage(const Msg &msg)
	{
		std::ostringstream os;
		msg.encode(os, 0, 0);
		const std::string raw = os.str();
		return write_exact(m_fd, raw.data(), raw.size(), IO_TIMEOUT_MS);
	}

	template <typename AbortFunc, typename Callback, typename ...Args>
	int
	waitForAnswer(AbortFunc af, Callback cb)
	{
		uint8_t payload[PAN_MAX_PAYLOAD];

		while (af()) {
			PanHeader hdr;

			int rc = read_pan_header(m_fd, &hdr, IO_TIMEOUT_MS);
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

			uint16_t len = hdr.len;

			if (len > 0) {
				rc = read_exact(m_fd, payload, len, IO_TIMEOUT_MS);
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

			if (cb(&hdr, payload, len) != 0)
			{
				return 1;
			}
		}

		return 0;
	}

  	int
	choosePacmanRole()
	{
		int send_status = sendMessage(bmsg::CL_role_choose { "pacman" });
		if ( send_status != 0) {
			fprintf(stderr, "send pacman role failed\n");
			return send_status;
		}

		bool role_chosen = false;
		int choice_status = waitForAnswer(
			[&role_chosen](){return !role_chosen;},
			[this, &role_chosen](PanHeader *hdr, const uint8_t *payload, uint16_t len){
				if (strcmp(hdr->prefix, "role") == 0) {
					if (strcmp(hdr->type, "chosen") == 0) {
						role_chosen = true;
					}
					return handleRoleMsg(hdr, payload, len);
				}

				return 0;
			}
		);

		return choice_status;
	}

	int
	handleRoleMsg(PanHeader *hdr, const uint8_t *payload, uint16_t len)
	{
		(void)len;
		const std::string raw = makeRawMessage(*hdr, payload);
		bmsg::RawMessage msg(raw);

		if ( strcmp(hdr->type, "chosen") == 0 )
		{
			return bmsg::SV_role_chosen::decode(msg) ? 0 : 1;
		} else if ( strcmp(hdr->type, "option") == 0 )
		{
			return bmsg::SV_role_option::decode(msg) ? 0 : 1;
		} else if ( strcmp(hdr->type, "reject") == 0 )
		{
			const auto reject = bmsg::SV_role_reject::decode(msg);
			if (reject) {
				fprintf(stderr, "role choice reject, reason: %.*s\n", (int)reject->reason.size(), reject->reason.data());
			} else {
				fprintf(stderr, "role choice reject\n");
			}
			return 1;
		}

		return 1;
	}

	int
	handlePacmanMsg(const char *type, const uint8_t *payload, uint16_t len)
	{
		PanHeader hdr;
		memset(&hdr, 0, sizeof(hdr));
		strcpy(hdr.prefix, "pacman");
		strncpy(hdr.type, type, sizeof(hdr.type) - 1);
		hdr.len = len;
		const std::string raw = makeRawMessage(hdr, payload);
		bmsg::RawMessage msg(raw);

		if (strcmp(type, "tick") == 0) {
			return bmsg::SV_pacman_tick::decode(msg) ? tick() : 0;
		}

		if (strcmp(type, "hp") == 0) {
			const auto hp = bmsg::SV_pacman_hp::decode(msg);
			if (!hp) {
				return 0;
			}

			m_hp = hp->val;
			fprintf(stderr, "hp = %d\n", m_hp);

			if (m_hp <= 0) {
				fprintf(stderr, "dead; closing connection\n");
				m_alive = 0;
			}

			return 0;
		}

		if (strcmp(type, "at") == 0) {
			const auto at = bmsg::SV_pacman_at::decode(msg);
			if (!at) {
				return 0;
			}
			m_coord.x = at->x;
			m_coord.y = at->y;
			m_have_pos = 1;
			fprintf(stderr, "at = (%d, %d)\n", m_coord.x, m_coord.y);
			return 0;
		}

		if (strcmp(type, "sees") == 0) {
			const auto sees = bmsg::SV_pacman_sees::decode(msg);
			if (!sees) {
				return 0;
			}

			m_visible_ids.insert(sees->who);
			fprintf(stderr, "sees id=%u at (%d, %d)\n", sees->who, sees->x, sees->y);
			return 0;
		}

		if (strcmp(type, "wall") == 0) {
			const auto wall = bmsg::SV_pacman_wall::decode(msg);
			if (!wall) {
				return 0;
			}
			m_walls.insert({wall->x, wall->y});
			fprintf(stderr, "wall at (%d, %d)\n", wall->x, wall->y);
			return 0;
		}		

		return 0;
	}

	int
	handleSrvMsg(const char *type, const uint8_t *payload, uint16_t len)
	{
		PanHeader hdr;
		memset(&hdr, 0, sizeof(hdr));
		strcpy(hdr.prefix, "srv");
		strncpy(hdr.type, type, sizeof(hdr.type) - 1);
		hdr.len = len;
		const std::string raw = makeRawMessage(hdr, payload);
		bmsg::RawMessage msg(raw);

		if (strcmp(type, "hasPref") == 0) {
			const auto has_pref = bmsg::SV_srv_hasPref::decode(msg);
			if (!has_pref) {
				return 0;
			}

			bmsg::Char64 pref_value = has_pref->pref;
			const std::string_view pref = pref_value;
			fprintf(stderr, "srv:hasPref pref='%.*s'\n", (int)pref.size(), pref.data());
			return 0;
		}

		if (strcmp(type, "name") == 0) {
			const auto name = bmsg::SV_srv_name::decode(msg);
			if (!name) {
				return 0;
			}

			fprintf(stderr, "srv:name name='%.*s'\n", (int)name->name.size(), name->name.data());
			return 0;
		}

		if (strcmp(type, "id") == 0) {
			const auto id = bmsg::SV_srv_id::decode(msg);
			if (!id) {
				return 0;
			}

			fprintf(stderr, "srv:id clientId=%u\n", id->clientId);
			return 0;
		}

		if (strcmp(type, "level") == 0) {
			const auto level = bmsg::SV_srv_level::decode(msg);
			if (!level) {
				return 0;
			}

			bmsg::Char64 level_value = level->level;
			const std::string_view level_view = level_value;
			fprintf(stderr, "srv:level level='%.*s'\n", (int)level_view.size(), level_view.data());
			return 0;
		}

		if (strcmp(type, "r.setLvl") == 0) {
			const auto set_level = bmsg::SV_srv_r_setLvl::decode(msg);
			if (!set_level) {
				return 0;
			}

			fprintf(stderr, "srv:r.setLvl msg=%u ok=%d\n", set_level->msg, set_level->ok);
			return 0;
		}

		fprintf(stderr, "ignoring unknown srv:%s len=%u\n", type, (unsigned)len);
		return 0;
	}

	int
	tick()
	{
		if (!m_alive) {
			return 0;
		}

		static const int8_t dirs[4][2] = {
			{  1,  0 },
			{ -1,  0 },
			{  0,  1 },
			{  0, -1 },
		};

		int8_t legal[4][2];
		size_t legal_count = 0;

		for (size_t i = 0; i < 4; i++) {
			int32_t nx = m_coord.x + dirs[i][0];
			int32_t ny = m_coord.y + dirs[i][1];

			if (m_have_pos && m_walls.count({nx, ny})) {
				continue;
			}

			legal[legal_count][0] = dirs[i][0];
			legal[legal_count][1] = dirs[i][1];
			legal_count++;
		}

		if (legal_count == 0) {
			fprintf(stderr, "boxed in by known walls; skipping tick\n");
			return 0;
		}

		size_t idx = (size_t)(rand() % (int)legal_count);
		int8_t dx = legal[idx][0];
		int8_t dy = legal[idx][1];

		if (sendMove(dx, dy) != 0) {
			fprintf(stderr, "send move failed\n");
			m_alive = 0;
		} else {
			fprintf(stderr, "move %d %d\n", (int)dx, (int)dy);
		}

		return 0;
	}

	int
	sendMove(int8_t dx, int8_t dy)
	{
		return sendMessage(bmsg::CL_pacman_move { dx, dy });
	}
};

int main(int argc, char **argv) {
	if (argc != 3) {
		fprintf(stderr, "usage: %s HOST PORT\n", argv[0]);
		return 1;
	}

	int fd = connect_tcp(argv[1], argv[2]);
	if (fd < 0) {
		return 1;
	}

	if (Pacman().run(fd) != 0)
	{
		fprintf(stderr, "Pacman().run faield\n");
	}

	close(fd);
	return 0;
}
