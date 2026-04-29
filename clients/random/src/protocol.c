#include "protocol.h"

#include <string.h>

uint16_t rd_u16_le(const uint8_t *src) {
    return (uint16_t)src[0] | ((uint16_t)src[1] << 8);
}

uint32_t rd_u32_le(const uint8_t *src) {
    return ((uint32_t)src[0]) |
        ((uint32_t)src[1] << 8) |
        ((uint32_t)src[2] << 16) |
        ((uint32_t)src[3] << 24);
}

int32_t rd_i32_le(const uint8_t *src) {
    return (int32_t)rd_u32_le(src);
}

void wr_u16_le(uint8_t *dst, uint16_t src) {
    dst[0] = (uint8_t)(src & 0xffu);
    dst[1] = (uint8_t)((src >> 8) & 0xffu);
}

void wr_u32_le(uint8_t *dst, uint32_t src) {
    dst[0] = (uint8_t)(src & 0xffu);
    dst[1] = (uint8_t)((src >> 8) & 0xffu);
    dst[2] = (uint8_t)((src >> 16) & 0xffu);
    dst[3] = (uint8_t)((src >> 24) & 0xffu);
}

int rd_bool_u8(const uint8_t *src) {
    return src[0] != 0;
}

void rd_char64(char out[9], const uint8_t *src) {
    memcpy(out, src, 8);
    out[8] = '\0';
}

int rd_pan_string(const uint8_t *payload, uint16_t len, char *out, size_t out_cap) {
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

int rd_pan_string_at(const uint8_t *payload, uint16_t len, uint16_t *off, char *out, size_t out_cap) {
    if (*off + 2 > len || out_cap == 0) {
        return -1;
    }

    uint16_t slen = rd_u16_le(payload + *off);
    if ((uint32_t)(*off) + 2u + (uint32_t)slen > (uint32_t)len) {
        return -1;
    }

    size_t copy_n = slen;
    if (copy_n >= out_cap) {
        copy_n = out_cap - 1;
    }

    memcpy(out, payload + *off + 2, copy_n);
    out[copy_n] = '\0';
    *off = (uint16_t)(*off + 2 + slen);
    return (int)slen;
}

void fill_name8(uint8_t out[PAN_NAME_LEN], const char *s) {
    memset(out, 0, PAN_NAME_LEN);
    size_t n = 0;
    while (n < PAN_NAME_LEN && s[n] != '\0') n++;
    memcpy(out, s, n);
}

void unpack_name8(char out[PAN_NAME_LEN + 1], const uint8_t in[PAN_NAME_LEN]) {
    size_t n = PAN_NAME_LEN;
    while (n > 0 && in[n - 1] == 0) {
        n--;
    }
    memcpy(out, in, n);
    out[n] = '\0';
}
