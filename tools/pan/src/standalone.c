#include "libpan.h"
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

static void l_help(const char *exec)
{
    puts("");
    printf("Usage: %s [OPTIONS]... [FILES]...\n", exec);
    puts("Parse binmsg & textmsg dumps, generate headers");
    puts("");
    puts("Action is determined by filetype:");
    puts("    dir       will search for .pan files there");
    puts("    .pan      protocol defenitions will be loaded");
    puts("    .bmsg     binmsg PRIVATEdump will be processed and dumped");
    puts("    .h, .hpp  will write header with all currently loaded message types");
    puts("");
    puts("Options are:");
    puts("    -h        Prints this help message");
    puts("    -c        .bmsg dumps are assumed to be from client");
    puts("    -s        .bmsg dumps are assumed to be from server");
    puts("    -i INC    .h files will include that file for macros");
    puts("    -l        Single-line dump outputs");
    puts("");
    puts("2026, (c) Ivan Didyk, github.com/random-username-here");
    puts("Standard for binmsg availiable at github.com/random-username-here/mipt-ded-bardak");
    puts("");
}

static void l_logger(const char *fmt, ...) {
    va_list args, copy;
    va_start(args, fmt);
    va_copy(copy, args);
    size_t size = vsnprintf(NULL, 0, fmt, copy) + 1;
    char *buf = malloc(size);
    assert(buf);
    vsnprintf(buf, size, fmt, args);
    va_end(copy);
    va_end(args);
    fputs(" -> ", stdout);
    for (const char *b = buf; *b; ++b) {
        if (*b == '\n') fputs("\n    ", stdout);
        else fputc(*b, stdout);
    }
    fputs("\n", stdout);
    free(buf);
}

static bool l_hasExt(const char *name, const char *ext)
{
    size_t len = strlen(name), elen = strlen(ext);
    if (elen > len) return false;
    return strcmp(name + len - elen, ext) == 0;
}

static char *l_readFile(const char *name, size_t *r_size)
{
    FILE *f = fopen(name, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END); // FIXME: handle failiures
    size_t size = ftell(f);
    char *buf = malloc(size);
    if (!buf) return NULL;
    fseek(f, 0, SEEK_SET);
    fread(buf, 1, size, f);
    fclose(f);
    if (r_size) *r_size = size;
    return buf;
}

static void l_process(const char *name, struct PAN *pan, enum PAN_Side side, bool isTop, const char *headpath, bool oneline)
{
    struct stat st = {0};
    if (stat(name, &st) != 0) {
        if ((l_hasExt(name, ".h") || l_hasExt(name, ".hpp")) && errno == ENOENT) {
            // we are generating header, it may not exist yet
            goto header;
        } else {
            printf("[x] Failed to stat %s: %s\n", name, strerror(errno));
            return;
        }
    }
    if (S_ISDIR(st.st_mode)) {
        // TODO
    } else if (!S_ISREG(st.st_mode)) {
        if (isTop)
            printf("[x] File %s is of unknown type\n", name);
    } else if (l_hasExt(name, ".pan")) {
        printf("Loading defs from %s\n", name);
        bool ok = pan_loadDefsFromFile(pan, name);
        if (!ok) {
            printf("[x] Failed to load defs: %s\n", strerror(errno));
        } else {
            puts("Defs loaded");
        }
    } else if (!isTop) {
        // skip
    } else if (l_hasExt(name, ".bmsg")) {
        printf("Dumping binmsg from %s\n", name);
        size_t fsz = 0;
        char *buf = l_readFile(name, &fsz);
        if (!buf) {
            printf("[x] Failed to read bmsg file: %s\n", strerror(errno));
            return;
        }
        puts("");
        size_t pos = 0;
        while (pos < fsz) 
            pos += (oneline ? pan_binDump_short : pan_binDump)(pan, side, buf + pos, fsz - pos);
        free(buf);
    } else if (l_hasExt(name, ".h") || l_hasExt(name, ".hpp")) {
    header:
        printf("Writting header to %s\n", name);
        FILE *output = fopen(name, "w");
        if (!output) {
            perror("Failed to open header for writting");
            return;
        }
        pan_generateHeader(pan, output, headpath);
    } else {
        printf("[x] Program doesn't know what to do with %s\n", strerror(errno));
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        l_help(argv[0]);
        return 0;
    }

    struct PAN pan;
    pan_init(&pan, l_logger, true);

    enum PAN_Side side = PAN_CLIENT;
    const char *header = "libpan_cxx_macros.hpp";
    bool oneline = false;

    int opt;
    while ((opt = getopt(argc, argv, "hcsli:")) != -1) {
        switch (opt) {
            case 'h':
                l_help(argv[0]);
                return 0;
            case 'c':
                side = PAN_CLIENT;
                break;
            case 's':
                side = PAN_SERVER;
                break;
            case 'i':
                header = optarg;
                break;
            case 'l':
                oneline = true;
                break;
            default:
                printf("Unknown argument `%c`!\n", opt);
                printf("Known arguments are: -h, -c, -s, -i INCPATH\n");
                return 1;
        }
    }

    for (size_t i = optind; i < argc; ++i)
        l_process(argv[i], &pan, side, true, header, oneline);

    pan_destroy(&pan);
    return 0;
}
