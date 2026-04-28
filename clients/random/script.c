#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

enum {
	PAN_NAME_LEN       = 8,
	PAN_HEADER_LEN     = 8 + 8 + 4 + 2 + 2,
	PAN_MAX_PAYLOAD    = 65535,
	IO_TIMEOUT_MS      = 250,
	CONNECT_TIMEOUT_S  = 5,
	MAX_VISIBLE        = 256,
	MAX_WALLS          = 8192
};

typedef struct {
	int32_t x;
	int32_t y;
} Pos;

typedef struct {
	char prefix[9];
	char type[9];
	uint32_t id;
	uint16_t len;
	uint16_t flags;
} PanHeader;

typedef struct {
	uint32_t id;
	int32_t x;
	int32_t y;
} VisibleUnit;

typedef struct {
	int fd;

	int alive;
	int have_pos;
	int32_t hp;
	int32_t self_x;
	int32_t self_y;

	VisibleUnit visible[MAX_VISIBLE];
	size_t visible_count;

	Pos walls[MAX_WALLS];
	size_t wall_count;
} BotState;

/* --------- Pack helpers --------- */

static uint16_t rd_u16_le(const uint8_t *src) {
	return (uint16_t)src[0] | ((uint16_t)src[1] << 8);
}
static uint16_t rd_u16_be(const uint8_t *src) {
	return (uint16_t)src[1] | ((uint16_t)src[0] << 8);
}

static uint32_t rd_u32_le(const uint8_t *src) {
	return ((uint32_t)src[0]) |
		((uint32_t)src[1] << 8) |
		((uint32_t)src[2] << 16) |
		((uint32_t)src[3] << 24);
}

static int32_t rd_i32_le(const uint8_t *src) {
	return (int32_t)rd_u32_le(src);
}

static void wr_u16_le(uint8_t *dst, uint16_t src) {
	dst[0] = (uint8_t)(src & 0xffu);
	dst[1] = (uint8_t)((src >> 8) & 0xffu);
}

static void wr_u32_le(uint8_t *dst, uint32_t src) {
	dst[0] = (uint8_t)(src & 0xffu);
	dst[1] = (uint8_t)((src >> 8) & 0xffu);
	dst[2] = (uint8_t)((src >> 16) & 0xffu);
	dst[3] = (uint8_t)((src >> 24) & 0xffu);
}


static int rd_bool_u8(const uint8_t *src) {
	return src[0] != 0;
}

static void rd_char64(char out[9], const uint8_t *src) {
	memcpy(out, src, 8);
	out[8] = '\0';

	/* trim trailing zero padding if present */
	for (int i = 7; i >= 0; --i) {
		if (out[i] == '\0') {
			continue;
		}
		break;
	}
}

static int rd_pan_string(const uint8_t *payload, uint16_t len, char *out, size_t out_cap) {
	if (len < 2 || out_cap == 0) {
		return -1;
	}

	uint16_t slen = rd_u16_le(payload);
	if ((uint32_t)slen + 2u != (uint32_t)len) {
		return -1;
	}

	size_t copy_n = slen;
	if (copy_n >= out_cap) {
		copy_n = out_cap - 1;
	}

	memcpy(out, payload + 2, copy_n);
	out[copy_n] = '\0';
	return (int)slen;
}

static int wait_fd(int fd, int want_write, int timeout_ms) {
	for (;;) {
		fd_set rfds, wfds;
		struct timeval tv;

		FD_ZERO(&rfds);
		FD_ZERO(&wfds);

		if (want_write) {
			FD_SET(fd, &wfds);
		} else {
			FD_SET(fd, &rfds);
		}

		tv.tv_sec = timeout_ms / 1000;
		tv.tv_usec = (timeout_ms % 1000) * 1000;

		int rc = select(fd + 1, want_write ? NULL : &rfds, want_write ? &wfds : NULL, NULL, &tv);
		if (rc > 0) {
			return 0;
		}
		if (rc == 0) {
			return 1; /* timeout */
		}
		if (errno == EINTR) {
			continue;
		}
		return -1;
	}
}

static int read_exact(int fd, void *buf, size_t len, int timeout_ms) {
	uint8_t *p = (uint8_t *)buf;
	size_t off = 0;

	while (off < len) {
		int rc = wait_fd(fd, 0, timeout_ms);
		if (rc != 0) {
			return rc;
		}

		ssize_t n = recv(fd, p + off, len - off, 0);
		if (n == 0) {
			return 2; /* EOF */
		}
		if (n < 0) {
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
				continue;
			}
			return -1;
		}

		off += (size_t)n;
	}

	return 0;
}

static int write_exact(int fd, const void *buf, size_t len, int timeout_ms) {
	const uint8_t *p = (const uint8_t *)buf;
	size_t off = 0;

	while (off < len) {
		int rc = wait_fd(fd, 1, timeout_ms);
		if (rc != 0) {
			return rc;
		}

		ssize_t n = send(fd, p + off, len - off, 0);
		if (n < 0) {
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
				continue;
			}
			return -1;
		}

		off += (size_t)n;
	}

	return 0;
}

static void fill_name8(uint8_t out[PAN_NAME_LEN], const char *s) {
	memset(out, 0, PAN_NAME_LEN);
	size_t n = strnlen(s, PAN_NAME_LEN);
	memcpy(out, s, n);
}

static void unpack_name8(char out[PAN_NAME_LEN + 1], const uint8_t in[PAN_NAME_LEN]) {
	size_t n = PAN_NAME_LEN;
	while (n > 0 && in[n - 1] == 0) {
		n--;
	}
	memcpy(out, in, n);
	out[n] = '\0';
}

static int send_frame(int fd, const char *prefix, const char *type,
                      uint32_t msg_id, uint16_t flags,
                      const void *payload, uint16_t payload_len) {
	uint8_t header[PAN_HEADER_LEN];

	fill_name8(header + 0, prefix);
	fill_name8(header + 8, type);
	wr_u32_le(header + 16, msg_id);
	wr_u16_le(header + 20, payload_len);
	wr_u16_le(header + 22, flags);

	if (write_exact(fd, header, sizeof(header), IO_TIMEOUT_MS) != 0) {
		return -1;
	}
	if (payload_len > 0 && payload != NULL) {
		if (write_exact(fd, payload, payload_len, IO_TIMEOUT_MS) != 0) {
			return -1;
		}
	}

	return 0;
}

static int send_person_move(int fd, int8_t dx, int8_t dy) {
	uint8_t payload[2];
	payload[0] = (uint8_t)dx;
	payload[1] = (uint8_t)dy;
	return send_frame(fd, "person", "move", 0, 0, payload, (uint16_t)sizeof(payload));
}

static int send_person_attack(int fd, uint32_t whom) {
	uint8_t payload[4];
	wr_u32_le(payload, whom);
	return send_frame(fd, "person", "attack", 0, 0, payload, (uint16_t)sizeof(payload));
}

static int connect_tcp(const char *host, const char *port) {
	struct addrinfo hints;
	struct addrinfo *res = NULL, *it;
	int fd = -1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_UNSPEC;

	int gai = getaddrinfo(host, port, &hints, &res);
	if (gai != 0) {
		fprintf(stderr, "getaddrinfo(%s, %s): %s\n", host, port, gai_strerror(gai));
		return -1;
	}

	for (it = res; it != NULL; it = it->ai_next) {
		fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
		if (fd < 0) {
			continue;
		}

		if (connect(fd, it->ai_addr, it->ai_addrlen) == 0) {
			break;
		}

		close(fd);
		fd = -1;
	}

	freeaddrinfo(res);

	if (fd < 0) {
		fprintf(stderr, "connect(%s, %s) failed\n", host, port);
		return -1;
	}

	(void)CONNECT_TIMEOUT_S;

	return fd;
}

static int wall_known(const BotState *st, int32_t x, int32_t y) {
	for (size_t i = 0; i < st->wall_count; i++) {
		if (st->walls[i].x == x && st->walls[i].y == y) {
			return 1;
		}
	}
	return 0;
}

static void add_wall(BotState *st, int32_t x, int32_t y) {
	if (wall_known(st, x, y)) {
		return;
	}
	if (st->wall_count < MAX_WALLS) {
		st->walls[st->wall_count].x = x;
		st->walls[st->wall_count].y = y;
		st->wall_count++;
	}
}

static void add_visible(BotState *st, uint32_t id, int32_t x, int32_t y) {
	for (size_t i = 0; i < st->visible_count; i++) {
		if (st->visible[i].id == id) {
			st->visible[i].x = x;
			st->visible[i].y = y;
			return;
		}
	}

	if (st->visible_count < MAX_VISIBLE) {
		st->visible[st->visible_count].id = id;
		st->visible[st->visible_count].x = x;
		st->visible[st->visible_count].y = y;
		st->visible_count++;
	}
}

static void clear_tick_state(BotState *st) {
	st->visible_count = 0;
}

static void do_random_action(BotState *st) {
	if (!st->alive) {
		return;
	}

	VisibleUnit attackable[MAX_VISIBLE];
	size_t attackable_count = 0;

	if (st->have_pos) {
		for (size_t i = 0; i < st->visible_count; i++) {
			int32_t dx = st->visible[i].x - st->self_x;
			int32_t dy = st->visible[i].y - st->self_y;

			if (dx < 0) dx = -dx;
			if (dy < 0) dy = -dy;

			if (dx <= 1 && dy <= 1) {
				attackable[attackable_count++] = st->visible[i];
			}
		}
	}

	if (attackable_count > 0) {
		size_t idx = (size_t)(rand() % (int)attackable_count);
		uint32_t target = attackable[idx].id;

		if (send_person_attack(st->fd, target) != 0) {
			fprintf(stderr, "send attack failed\n");
			st->alive = 0;
		} else {
			fprintf(stderr, "attack %u\n", target);
		}

		clear_tick_state(st);
		return;
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
		int32_t nx = st->self_x + dirs[i][0];
		int32_t ny = st->self_y + dirs[i][1];

		if (st->have_pos && wall_known(st, nx, ny)) {
			continue;
		}

		legal[legal_count][0] = dirs[i][0];
		legal[legal_count][1] = dirs[i][1];
		legal_count++;
	}

	if (legal_count == 0) {
		fprintf(stderr, "boxed in by known walls; skipping tick\n");
		clear_tick_state(st);
		return;
	}

	size_t idx = (size_t)(rand() % (int)legal_count);
	int8_t dx = legal[idx][0];
	int8_t dy = legal[idx][1];

	if (send_person_move(st->fd, dx, dy) != 0) {
		fprintf(stderr, "send move failed\n");
		st->alive = 0;
	} else {
		fprintf(stderr, "move %d %d\n", (int)dx, (int)dy);
	}

	clear_tick_state(st);
}

static void handle_person_message(BotState *st, const char *type, const uint8_t *payload, uint16_t len) {
	if (strcmp(type, "tick") == 0) {
		if (len != 0) {
			return;
		}
		do_random_action(st);
		return;
	}

	if (strcmp(type, "hp") == 0) {
		if (len != 4) {
			return;
		}
		st->hp = rd_i32_le(payload);
		fprintf(stderr, "hp = %d\n", st->hp);

		if (st->hp <= 0) {
			fprintf(stderr, "dead; closing connection\n");
			st->alive = 0;
		}
		return;
	}

	if (strcmp(type, "at") == 0) {
		if (len != 8) {
			return;
		}
		st->self_x = rd_i32_le(payload + 0);
		st->self_y = rd_i32_le(payload + 4);
		st->have_pos = 1;
		fprintf(stderr, "at = (%d, %d)\n", st->self_x, st->self_y);
		return;
	}

	if (strcmp(type, "sees") == 0) {
		if (len != 12) {
			return;
		}

		int32_t x = rd_i32_le(payload + 0);
		int32_t y = rd_i32_le(payload + 4);
		uint32_t who = rd_u32_le(payload + 8);

		(void)x;
		(void)y;

		add_visible(st, who, x, y);
		fprintf(stderr, "sees id=%u at (%d, %d)\n", who, x, y);
		return;
	}

	if (strcmp(type, "wall") == 0) {
		if (len != 8) {
			return;
		}
		int32_t x = rd_i32_le(payload + 0);
		int32_t y = rd_i32_le(payload + 4);
		add_wall(st, x, y);
		fprintf(stderr, "wall at (%d, %d)\n", x, y);
		return;
	}

	/* ignore unknown person:* messages */
}

static void handle_srv_message(BotState *st, const char *type, const uint8_t *payload, uint16_t len) {
	(void)st;

	if (strcmp(type, "hasPref") == 0) {
		if (len != 8) {
			fprintf(stderr, "bad srv:hasPref len=%u\n", (unsigned)len);
			return;
		}

		char pref[9];
		rd_char64(pref, payload);
		fprintf(stderr, "srv:hasPref pref='%s'\n", pref);
		return;
	}

	if (strcmp(type, "name") == 0) {
		char name[1024];
		if (rd_pan_string(payload, len, name, sizeof(name)) < 0) {
			fprintf(stderr, "bad srv:name len=%u\n", (unsigned)len);
			return;
		}

		fprintf(stderr, "srv:name name='%s'\n", name);
		return;
	}

	if (strcmp(type, "id") == 0) {
		if (len != 4) {
			fprintf(stderr, "bad srv:id len=%u\n", (unsigned)len);
			return;
		}

		uint32_t client_id = rd_u32_le(payload);
		fprintf(stderr, "srv:id clientId=%u\n", client_id);
		return;
	}

	if (strcmp(type, "level") == 0) {
		if (len != 8) {
			fprintf(stderr, "bad srv:level len=%u\n", (unsigned)len);
			return;
		}

		char level[9];
		rd_char64(level, payload);
		fprintf(stderr, "srv:level level='%s'\n", level);
		return;
	}

	if (strcmp(type, "r.setLvl") == 0) {
		if (len != 5) {
			fprintf(stderr, "bad srv:r.setLvl len=%u\n", (unsigned)len);
			return;
		}

		uint32_t msg = rd_u32_le(payload + 0);
		int ok = rd_bool_u8(payload + 4);
		fprintf(stderr, "srv:r.setLvl msg=%u ok=%d\n", msg, ok);
		return;
	}

	fprintf(stderr, "ignoring unknown srv:%s len=%u\n", type, (unsigned)len);
}

static int read_pan_header(int fd, PanHeader *hdr, int timeout_ms) {
	uint8_t raw[PAN_HEADER_LEN];
	int rc = read_exact(fd, raw, sizeof(raw), timeout_ms);
	if (rc != 0) {
		return rc;
	}

	unpack_name8(hdr->prefix, raw + 0);
	unpack_name8(hdr->type,   raw + 8);
	hdr->id    = rd_u32_le(raw + 16);
	hdr->len   = rd_u16_le(raw + 20);
	hdr->flags = rd_u16_le(raw + 22);
	return 0;
}

int main(int argc, char **argv) {
	if (argc != 3) {
		fprintf(stderr, "usage: %s HOST PORT\n", argv[0]);
		return 1;
	}

	srand((unsigned int)(time(NULL) ^ (unsigned int)getpid()));

	int fd = connect_tcp(argv[1], argv[2]);
	if (fd < 0) {
		return 1;
	}

	BotState st;
	memset(&st, 0, sizeof(st));
	st.fd = fd;
	st.alive = 1;
	st.hp = 1;

	uint8_t header[PAN_HEADER_LEN];
	uint8_t payload[PAN_MAX_PAYLOAD];

	while (st.alive) {
		PanHeader hdr;

		int rc = read_pan_header(fd, &hdr, IO_TIMEOUT_MS);
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
			rc = read_exact(fd, payload, len, IO_TIMEOUT_MS);
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

		if (strcmp(hdr.prefix, "person") == 0) {
			handle_person_message(&st, hdr.type, payload, len);
		} else if (strcmp(hdr.prefix, "srv") == 0) {
			handle_srv_message(&st, hdr.type, payload, len);
		}
	}

	close(fd);
	return 0;
}
