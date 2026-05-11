#define _POSIX_C_SOURCE 200809L

#include "net.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int wait_fd(int fd, int want_write, int timeout_ms) {
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
            return 1;
        }
        if (errno == EINTR) {
            continue;
        }
        return -1;
    }
}

int read_exact(int fd, void *buf, size_t len, int timeout_ms) {
    uint8_t *p = (uint8_t *)buf;
    size_t off = 0;

    while (off < len) {
        int rc = wait_fd(fd, 0, timeout_ms);
        if (rc != 0) {
            return rc;
        }

        ssize_t n = recv(fd, p + off, len - off, 0);
        if (n == 0) {
            return 2;
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

int write_exact(int fd, const void *buf, size_t len, int timeout_ms) {
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

int send_frame(int fd, const char *prefix, const char *type,
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

int connect_tcp(const char *host, const char *port) {
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

int read_pan_header(int fd, PanHeader *hdr, int timeout_ms) {
    uint8_t raw[PAN_HEADER_LEN];
    int rc = read_exact(fd, raw, sizeof(raw), timeout_ms);
    if (rc != 0) {
        return rc;
    }

    unpack_name8(hdr->prefix, raw + 0);
    unpack_name8(hdr->type, raw + 8);
    hdr->id = rd_u32_le(raw + 16);
    hdr->len = rd_u16_le(raw + 20);
    hdr->flags = rd_u16_le(raw + 22);
    return 0;
}
