#include "../utils/tcp_connection.h"
#include <cstdio>
#include <cstring>
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

	int m_fd;

	bool m_alive;
	int m_have_pos;

	int32_t m_hp;
	Pos m_coord;
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
		std::string pacman_role_id = "pacman";
		int send_status = send_frame(m_fd, "role", "choose", 0, 0, pacman_role_id.data(), pacman_role_id.size());
		if ( send_status != 0) {
			fprintf(stderr, "send pacman role failed\n");
			return send_status;
		}

		int choice_status = waitForAnswer(
			[](){return true;},
			[this](PanHeader *hdr, const uint8_t *payload, uint16_t len){
				if (strcmp(hdr->prefix, "role") == 0) {
					return handleRoleMsg(hdr->type, payload, len);
				}

				return 0;
			}
		);

		return choice_status;
	}

	int
	handleRoleMsg(const char *type, const uint8_t *payload, uint16_t len)
	{
		if ( strcmp(type, "chosen") == 0 )
		{
			return 0;
		} else if ( strcmp(type, "reject") == 0 )
		{
			PanHeader hdr;
			if (read_pan_header(m_fd, &hdr, IO_TIMEOUT_MS) != 0)
			{
				fprintf(stderr, "role choice reject, could not read reject reason\n");						
				return 1;
			}

			std::string reason;
			reason.resize(hdr.len);
			fprintf(stderr, "role choice reject, reason: %s\n", reason.data());						
			return 1;
		}

		return 1;
	}

	int
	handlePacmanMsg(const char *type, const uint8_t *payload, uint16_t len)
	{
		if (strcmp(type, "tick") == 0) {
			return (len != 0) ? 0 : tick();
		}

		if (strcmp(type, "hp") == 0) {
			if (len != 4) {
				return 0;
			}

			m_hp = rd_i32_le(payload);
			fprintf(stderr, "hp = %d\n", m_hp);

			if (m_hp <= 0) {
				fprintf(stderr, "dead; closing connection\n");
				m_alive = 0;
			}

			return 0;
		}

		if (strcmp(type, "at") == 0) {
			if (len != 8) {
				return 0;
			}
			m_coord.x = rd_i32_le(payload + 0);
			m_coord.y = rd_i32_le(payload + 4);
			m_have_pos = 1;
			fprintf(stderr, "at = (%d, %d)\n", m_coord.x, m_coord.y);
			return 0;
		}

		if (strcmp(type, "sees") == 0) {
			if (len != 12) {
				return 0;
			}

			int32_t x = rd_i32_le(payload + 0);
			int32_t y = rd_i32_le(payload + 4);
			uint32_t who = rd_u32_le(payload + 8);

			m_visible_ids.insert(who);
			fprintf(stderr, "sees id=%u at (%d, %d)\n", who, x, y);
			return 0;
		}

		if (strcmp(type, "wall") == 0) {
			if (len != 8) {
				return 0;
			}
			int32_t x = rd_i32_le(payload + 0);
			int32_t y = rd_i32_le(payload + 4);
			m_walls.insert({x, y});
			fprintf(stderr, "wall at (%d, %d)\n", x, y);
			return 0;
		}		

		return 0;
	}

	int
	handleSrvMsg(const char *type, const uint8_t *payload, uint16_t len)
	{
		if (strcmp(type, "hasPref") == 0) {
			if (len != 8) {
				fprintf(stderr, "bad srv:hasPref len=%u\n", (unsigned)len);
				return 0;
			}

			char pref[9];
			rd_char64(pref, payload);
			fprintf(stderr, "srv:hasPref pref='%s'\n", pref);
			return 0;
		}

		if (strcmp(type, "name") == 0) {
			char name[1024];
			if (rd_pan_string(payload, len, name, sizeof(name)) < 0) {
				fprintf(stderr, "bad srv:name len=%u\n", (unsigned)len);
				return 0;
			}

			fprintf(stderr, "srv:name name='%s'\n", name);
			return 0;
		}

		if (strcmp(type, "id") == 0) {
			if (len != 4) {
				fprintf(stderr, "bad srv:id len=%u\n", (unsigned)len);
				return 0;
			}

			uint32_t client_id = rd_u32_le(payload);
			fprintf(stderr, "srv:id clientId=%u\n", client_id);
			return 0;
		}

		if (strcmp(type, "level") == 0) {
			if (len != 8) {
				fprintf(stderr, "bad srv:level len=%u\n", (unsigned)len);
				return 0;
			}

			char level[9];
			rd_char64(level, payload);
			fprintf(stderr, "srv:level level='%s'\n", level);
			return 0;
		}

		if (strcmp(type, "r.setLvl") == 0) {
			if (len != 5) {
				fprintf(stderr, "bad srv:r.setLvl len=%u\n", (unsigned)len);
				return 0;
			}

			uint32_t msg = rd_u32_le(payload + 0);
			int ok = rd_bool_u8(payload + 4);
			fprintf(stderr, "srv:r.setLvl msg=%u ok=%d\n", msg, ok);
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
		uint8_t payload[2];
		payload[0] = (uint8_t)dx;
		payload[1] = (uint8_t)dy;
		return send_frame(m_fd, "pacman", "move", 0, 0, payload, (uint16_t)sizeof(payload));
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
