///                                    _       _     _____                   ///
///                          .-'+     | |   (_) |   | ___ \                  ///
///                       .-' .-'     | |    _| |__ | |_/ /_ _ _ __          ///
///          ,-------. .-'  .'        | |   | | '_ \|  __/ _` | '_ \         ///
///       ,-'         `-..-'          | |___| | |_) | | | (_| | | | |        ///
///      /    ,-----.    \            \_____/_|_.__/\_|  \__,_|_| |_|        ///
///     (   ,'       `.   )                                                  ///
///      \ |          |  /            A protocol analyzer library            ///
///       \-..       ,.-/             for textmsg & binmsg formats           ///
///        `. `-----' ,'              (c) 2026, Didyk Ivan,                  ///
///          '-------'                github.com/random-username-here        ///
///                                                                          ///
                                                                          
#ifndef I_LIBPAN_H
#define I_LIBPAN_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PAN_BINMSG_PREF_LEN 8
#define PAN_BINMSG_TYPE_LEN 8

#define PAN_HEXDUMP_WIDTH 8

enum PAN_Type {
    PAN_ID,
    PAN_CHAR64,
    PAN_INT8,
    PAN_INT16,
    PAN_INT32,
    PAN_INT64,
    PAN_FLOAT,
    PAN_DOUBLE,
    PAN_STRING,
    PAN_BLOB,
    PAN_BOOL
};

enum PAN_Side {
    PAN_CLIENT,
    PAN_SERVER
};

struct PAN_Arg {
    struct PAN_Arg *next;
    enum PAN_Type type;
    char *name;
};

struct PAN_MsgType {
    struct PAN_MsgType *prev;
    enum PAN_Side side;
    char *prefix;
    char *type;
    struct PAN_Arg *firstArg;
};

typedef void (*PAN_Logger)(const char *fmt, ...);

struct PAN {
    PAN_Logger logger;
    struct PAN_MsgType *msgTypes;
    bool color;
};


/** Initialize analyzer. Will use that logger or print to stderr if NULL is given. */
void pan_init(struct PAN *pan, PAN_Logger logger, bool color);

/** Load protocol defenitions from provided source */
bool pan_loadDefs(struct PAN *pan, const char *defs);

/** Load protocols defs from given file name */
bool pan_loadDefsFromFile(struct PAN *pan, const char *path);

/** Find message by prefix and type */
struct PAN_MsgType *pan_find(struct PAN *pan, enum PAN_Side side, const char *pref, const char *type);

/** Find message type corresponding to given message */
struct PAN_MsgType *pan_binMatch(struct PAN *pan, enum PAN_Side side, const void *msg, size_t len);

/** Print message to some form of output, returns number of bytes taken (in case there are multiple messages in sequence). */
size_t pan_binDump(struct PAN *pan, enum PAN_Side side, const void *msg, size_t len);

/** Print a message, but in one line. */
size_t pan_binDump_short(struct PAN *pan, enum PAN_Side side, const void *msg, size_t len);

/** Generate header for all loaded message types. You can run `pan examples/every.pan every.h` to see how resulting file will look. */
void pan_generateHeader(struct PAN *pan, FILE *output, const char *incpath);

/** Free protocol analyzer's structure */
void pan_destroy(struct PAN *pan);

#ifdef __cplusplus
};
#endif

#endif
