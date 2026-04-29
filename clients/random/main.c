#define _POSIX_C_SOURCE 200809L

#include "bot.h"
#include "net.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "usage: %s HOST PORT [ROLE_ID]\n", argv[0]);
        return 1;
    }

    const char *role_id = argc == 4 ? argv[3] : "pacman";

    srand((unsigned int)(time(NULL) ^ (unsigned int)getpid()));

    int fd = connect_tcp(argv[1], argv[2]);
    if (fd < 0) return 1;

    BotState st;
    bot_state_init(&st, fd, role_id);

    uint8_t payload[PAN_MAX_PAYLOAD];

    while (st.alive) {
        PanHeader hdr;

        int rc = read_pan_header(fd, &hdr, IO_TIMEOUT_MS);
        if (rc == 1) continue;
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
        } else if (strcmp(hdr.prefix, "role") == 0) {
            handle_role_message(&st, hdr.type, payload, len);
        } else {
            fprintf(stderr, "ignoring unknown prefix:%s type:%s len=%u\n", hdr.prefix, hdr.type, (unsigned)len);
        }
    }

    close(fd);
    return 0;
}
