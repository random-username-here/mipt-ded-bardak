#include "bot.h"
#include "net.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int wall_known(const BotState *st, int32_t x, int32_t y) {
    for (size_t i = 0; i < st->wall_count; i++) {
        if (st->walls[i].x == x && st->walls[i].y == y) return 1;
    }
    return 0;
}

static void add_wall(BotState *st, int32_t x, int32_t y) {
    if (wall_known(st, x, y)) return;
    if (st->wall_count < MAX_WALLS) {
        st->walls[st->wall_count].x = x;
        st->walls[st->wall_count].y = y;
        st->wall_count++;
    }
}

static void add_visible(BotState *st, uint32_t id) {
    for (size_t i = 0; i < st->visible_count; i++) {
        if (st->visible_ids[i] == id) return;
    }
    if (st->visible_count < MAX_VISIBLE) {
        st->visible_ids[st->visible_count++] = id;
    }
}

static void clear_tick_state(BotState *st) {
    st->visible_count = 0;
}

static void do_random_action(BotState *st) {
    if (!st->alive || !st->role_chosen) return;

    if (st->visible_count > 0) {
        size_t idx = (size_t)(rand() % (int)st->visible_count);
        uint32_t target = st->visible_ids[idx];
        if (send_person_attack(st->fd, target) != 0) {
            fprintf(stderr, "send attack failed\n");
            st->alive = 0;
        } else {
            fprintf(stderr, "attack %u\n", target);
        }
        clear_tick_state(st);
        return;
    }

    static const int8_t dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    int8_t legal[4][2];
    size_t legal_count = 0;

    for (size_t i = 0; i < 4; i++) {
        int32_t nx = st->self_x + dirs[i][0];
        int32_t ny = st->self_y + dirs[i][1];
        if (st->have_pos && wall_known(st, nx, ny)) continue;
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

static void try_choose_role(BotState *st) {
    if (st->role_choose_sent || st->role_chosen) return;
    if (st->desired_role_id[0] == '\0') return;

    for (size_t i = 0; i < st->role_count; ++i) {
        if (strcmp(st->roles[i].id, st->desired_role_id) == 0) {
            if (send_role_choose(st->fd, st->desired_role_id) != 0) {
                fprintf(stderr, "send role:choose failed\n");
                st->alive = 0;
            } else {
                fprintf(stderr, "sent role:choose id='%s'\n", st->desired_role_id);
                st->role_choose_sent = 1;
            }
            return;
        }
    }
}

void bot_state_init(BotState *st, int fd, const char *desired_role_id) {
    memset(st, 0, sizeof(*st));
    st->fd = fd;
    st->alive = 1;
    st->hp = 1;
    if (desired_role_id) {
        strncpy(st->desired_role_id, desired_role_id, sizeof(st->desired_role_id) - 1);
        st->desired_role_id[sizeof(st->desired_role_id) - 1] = '\0';
    }
}

int send_person_move(int fd, int8_t dx, int8_t dy) {
    uint8_t payload[2];
    payload[0] = (uint8_t)dx;
    payload[1] = (uint8_t)dy;
    return send_frame(fd, "person", "move", 0, 0, payload, (uint16_t)sizeof(payload));
}

int send_person_attack(int fd, uint32_t whom) {
    uint8_t payload[4];
    wr_u32_le(payload, whom);
    return send_frame(fd, "person", "attack", 0, 0, payload, (uint16_t)sizeof(payload));
}

int send_role_choose(int fd, const char *role_id) {
    size_t n = strlen(role_id);
    if (n > 65535u - 2u) return -1;

    uint16_t payload_len = (uint16_t)(2u + n);
    uint8_t *payload = malloc(payload_len);
    if (!payload) return -1;

    wr_u16_le(payload, (uint16_t)n);
    memcpy(payload + 2, role_id, n);

    int rc = send_frame(fd, "role", "choose", 0, 0, payload, payload_len);
    free(payload);
    return rc;
}

void handle_person_message(BotState *st, const char *type, const uint8_t *payload, uint16_t len) {
    if (strcmp(type, "tick") == 0) {
        if (len == 0) do_random_action(st);
        return;
    }

    if (strcmp(type, "hp") == 0) {
        if (len != 4) return;
        st->hp = rd_i32_le(payload);
        fprintf(stderr, "hp = %d\n", st->hp);
        if (st->hp <= 0) {
            fprintf(stderr, "dead; closing connection\n");
            st->alive = 0;
        }
        return;
    }

    if (strcmp(type, "at") == 0) {
        if (len != 8) return;
        st->self_x = rd_i32_le(payload + 0);
        st->self_y = rd_i32_le(payload + 4);
        st->have_pos = 1;
        fprintf(stderr, "at = (%d, %d)\n", st->self_x, st->self_y);
        return;
    }

    if (strcmp(type, "sees") == 0) {
        if (len != 12) return;
        int32_t x = rd_i32_le(payload + 0);
        int32_t y = rd_i32_le(payload + 4);
        uint32_t who = rd_u32_le(payload + 8);
        add_visible(st, who);
        fprintf(stderr, "sees id=%u at (%d, %d)\n", who, x, y);
        return;
    }

    if (strcmp(type, "wall") == 0) {
        if (len != 8) return;
        int32_t x = rd_i32_le(payload + 0);
        int32_t y = rd_i32_le(payload + 4);
        add_wall(st, x, y);
        fprintf(stderr, "wall at (%d, %d)\n", x, y);
        return;
    }
}

void handle_srv_message(BotState *st, const char *type, const uint8_t *payload, uint16_t len) {
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

void handle_role_message(BotState *st, const char *type, const uint8_t *payload, uint16_t len) {
    if (strcmp(type, "option") == 0) {
        uint16_t off = 0;
        char id[256];
        char name[256];
        char prefix[256];

        if (rd_pan_string_at(payload, len, &off, id, sizeof(id)) < 0 ||
            rd_pan_string_at(payload, len, &off, name, sizeof(name)) < 0 ||
            rd_pan_string_at(payload, len, &off, prefix, sizeof(prefix)) < 0 ||
            off != len) {
            fprintf(stderr, "bad role:option len=%u\n", (unsigned)len);
            return;
        }

        fprintf(stderr, "role:option id='%s' name='%s' prefix='%s'\n", id, name, prefix);

        if (st->role_count < sizeof(st->roles) / sizeof(st->roles[0])) {
            RoleOption *opt = &st->roles[st->role_count++];
            strncpy(opt->id, id, sizeof(opt->id) - 1);
            strncpy(opt->name, name, sizeof(opt->name) - 1);
            strncpy(opt->prefix, prefix, sizeof(opt->prefix) - 1);
            opt->id[sizeof(opt->id) - 1] = '\0';
            opt->name[sizeof(opt->name) - 1] = '\0';
            opt->prefix[sizeof(opt->prefix) - 1] = '\0';
        }

        try_choose_role(st);
        return;
    }

    if (strcmp(type, "chosen") == 0) {
        char id[256];
        if (rd_pan_string(payload, len, id, sizeof(id)) < 0) {
            fprintf(stderr, "bad role:chosen len=%u\n", (unsigned)len);
            return;
        }
        st->role_chosen = 1;
        fprintf(stderr, "role:chosen id='%s'\n", id);
        return;
    }

    if (strcmp(type, "reject") == 0) {
        char reason[512];
        if (rd_pan_string(payload, len, reason, sizeof(reason)) < 0) {
            fprintf(stderr, "bad role:reject len=%u\n", (unsigned)len);
            return;
        }
        fprintf(stderr, "role:reject reason='%s'\n", reason);
        st->alive = 0;
        return;
    }

    fprintf(stderr, "ignoring unknown role:%s len=%u\n", type, (unsigned)len);
}
