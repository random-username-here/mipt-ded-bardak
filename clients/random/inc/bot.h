#ifndef BOT_H
#define BOT_H

#include "protocol.h"

#include <stddef.h>
#include <stdint.h>

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

void bot_state_init(BotState *st, int fd);
void handle_person_message(BotState *st, const char *type, const uint8_t *payload, uint16_t len);
void handle_srv_message(BotState *st, const char *type, const uint8_t *payload, uint16_t len);

#endif
