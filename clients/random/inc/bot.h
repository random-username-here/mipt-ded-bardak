#ifndef BOT_H
#define BOT_H

#include "protocol.h"

#include <stddef.h>
#include <stdint.h>

typedef struct {
    char id[MAX_ROLE_ID_LEN + 1];
    char name[MAX_ROLE_NAME_LEN + 1];
    char prefix[MAX_ROLE_PREFIX_LEN + 1];
} RoleOption;

typedef struct {
    int fd;
    int alive;
    int have_pos;
    int32_t hp;
    int32_t self_x;
    int32_t self_y;
    uint32_t visible_ids[MAX_VISIBLE];
    size_t visible_count;
    Pos walls[MAX_WALLS];
    size_t wall_count;
    char desired_role_id[MAX_ROLE_ID_LEN + 1];
    int role_choose_sent;
    int role_chosen;
    RoleOption roles[32];
    size_t role_count;
} BotState;

void bot_state_init(BotState *st, int fd, const char *desired_role_id);
void handle_person_message(BotState *st, const char *type, const uint8_t *payload, uint16_t len);
void handle_srv_message(BotState *st, const char *type, const uint8_t *payload, uint16_t len);
void handle_role_message(BotState *st, const char *type, const uint8_t *payload, uint16_t len);
int send_person_move(int fd, int8_t dx, int8_t dy);
int send_person_attack(int fd, uint32_t whom);
int send_role_choose(int fd, const char *role_id);

#endif
