#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

enum {
    PAN_NAME_LEN       = 8,
    PAN_HEADER_LEN     = 8 + 8 + 4 + 2 + 2,
    PAN_MAX_PAYLOAD    = 65535,
    IO_TIMEOUT_MS      = 250,
    CONNECT_TIMEOUT_S  = 5,
    MAX_VISIBLE        = 256,
    MAX_WALLS          = 8192,
    MAX_ROLE_ID_LEN    = 255,
    MAX_ROLE_NAME_LEN  = 255,
    MAX_ROLE_PREFIX_LEN = 255
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

uint16_t rd_u16_le(const uint8_t *src);
uint32_t rd_u32_le(const uint8_t *src);
int32_t rd_i32_le(const uint8_t *src);
void wr_u16_le(uint8_t *dst, uint16_t src);
void wr_u32_le(uint8_t *dst, uint32_t src);
int rd_bool_u8(const uint8_t *src);
void rd_char64(char out[9], const uint8_t *src);
int rd_pan_string(const uint8_t *payload, uint16_t len, char *out, size_t out_cap);
int rd_pan_string_at(const uint8_t *payload, uint16_t len, uint16_t *off, char *out, size_t out_cap);
void fill_name8(uint8_t out[PAN_NAME_LEN], const char *s);
void unpack_name8(char out[PAN_NAME_LEN + 1], const uint8_t in[PAN_NAME_LEN]);

#endif
