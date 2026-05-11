#include "bot.h"
#include "net.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static int iabs32(int32_t v) {
    return v < 0 ? -v : v;
}

static int sign32(int32_t v) {
    if (v < 0) return -1;
    if (v > 0) return 1;
    return 0;
}

static int long_dist(int32_t ax, int32_t ay, int32_t bx, int32_t by) {
    int dx = iabs32(ax - bx);
    int dy = iabs32(ay - by);
    return dx > dy ? dx : dy;
}

static int try_move(BotState *st, int8_t dx, int8_t dy, const char *why) {
    if (dx == 0 && dy == 0) {
        return 0;
    }

    int32_t nx = st->self_x + dx;
    int32_t ny = st->self_y + dy;

    if (st->have_pos && wall_known(st, nx, ny)) {
        return 0;
    }

    if (send_person_move(st->fd, dx, dy) != 0) {
        fprintf(stderr, "send move failed\n");
        st->alive = 0;
        return 1;
    }

    fprintf(stderr, "%s move %d %d\n", why, (int)dx, (int)dy);
    return 1;
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

    if (st->have_pos && st->visible_count > 0) {
        size_t best = 0;
        int best_dist = long_dist(st->self_x, st->self_y, st->visible[0].x, st->visible[0].y);

        for (size_t i = 1; i < st->visible_count; i++) {
            int d = long_dist(st->self_x, st->self_y, st->visible[i].x, st->visible[i].y);
            if (d < best_dist) {
                best = i;
                best_dist = d;
            }
        }

        VisibleUnit target = st->visible[best];
        int8_t sx = (int8_t)sign32(target.x - st->self_x);
        int8_t sy = (int8_t)sign32(target.y - st->self_y);

        if (try_move(st, sx, sy, "chase")) {
            clear_tick_state(st);
            return;
        }

        if (iabs32(target.x - st->self_x) >= iabs32(target.y - st->self_y)) {
            if (try_move(st, sx, 0, "chase")) {
                clear_tick_state(st);
                return;
            }
            if (try_move(st, 0, sy, "chase")) {
                clear_tick_state(st);
                return;
            }
        } else {
            if (try_move(st, 0, sy, "chase")) {
                clear_tick_state(st);
                return;
            }
            if (try_move(st, sx, 0, "chase")) {
                clear_tick_state(st);
                return;
            }
        }
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

void bot_state_init(BotState *st, int fd) {
    memset(st, 0, sizeof(*st));
    st->fd = fd;
    st->alive = 1;
    st->hp = 1;
}

void handle_person_message(BotState *st, const char *type, const uint8_t *payload, uint16_t len) {
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
