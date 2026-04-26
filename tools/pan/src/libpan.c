#include "libpan.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ESC_GRY "\x1b[90m"
#define ESC_RED "\x1b[91m"
#define ESC_GRN "\x1b[92m"
#define ESC_YLW "\x1b[93m"
#define ESC_BLU "\x1b[94m"
#define ESC_MGN "\x1b[95m"
#define ESC_CYN "\x1b[96m"
#define ESC_RST "\x1b[0m"

//==============================================================================
// Initialization
//==============================================================================

static void l_defaultLogger(const char *fmt, ...) {
    va_list args;
    fputs("libpan: ", stderr);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputs("\n", stderr);
}

void pan_init(struct PAN *pan, PAN_Logger logger, bool color)
{
    assert(pan);
    pan->logger = logger ? logger : l_defaultLogger;
    pan->msgTypes = NULL;
    pan->color = color;
}

static void l_destroyArg(struct PAN_Arg *arg)
{
    if (!arg) return;
    l_destroyArg(arg->next);
    free(arg->name);
    free(arg);
}

static void l_destroyType(struct PAN_MsgType *type)
{
    if (!type) return;
    l_destroyArg(type->firstArg);
    l_destroyType(type->prev);
    free(type->prefix);
    free(type->type);
}

void pan_destroy(struct PAN *pan)
{
    if (!pan) return;
    l_destroyType(pan->msgTypes);
}

//==============================================================================
// .pan file parsing
//==============================================================================

enum TokType
{
    T_EOF = 0,
    T_DOTS = ':',
    T_LBRACE = '(',
    T_RBRACE = ')',
    T_COMMA = ',',
    T_SEMICOLON = ';',
    
    T_NAME,

    T_KW_CLIENT,
    T_KW_SERVER,

    // only types go below
    // order them the same way as PAN_Type
    KW_MIN_TYPE,
    T_KW_TYP_ID = KW_MIN_TYPE,
    T_KW_TYP_CHAR64,
    T_KW_TYP_INT8,
    T_KW_TYP_INT16,
    T_KW_TYP_INT32,
    T_KW_TYP_INT64,
    T_KW_TYP_FLOAT,
    T_KW_TYP_DOUBLE,
    T_KW_TYP_STRING,
    T_KW_TYP_BLOB,
    T_KW_TYP_BOOL,
};

struct Tok
{
    enum TokType type;
    const char *origin;
    size_t size, line;
};

struct Keyword
{
    const char *kw;
    enum TokType type;
};

static struct Keyword l_keywords[] =
{
    { "client", T_KW_CLIENT },
    { "server", T_KW_SERVER },
    { "id", T_KW_TYP_ID },
    { "char64", T_KW_TYP_CHAR64 },
    { "int8", T_KW_TYP_INT8 },
    { "int16", T_KW_TYP_INT16 },
    { "int32", T_KW_TYP_INT32 },
    { "int64", T_KW_TYP_INT64 },
    { "float", T_KW_TYP_FLOAT },
    { "double", T_KW_TYP_DOUBLE },
    { "string", T_KW_TYP_STRING },
    { "blob", T_KW_TYP_BLOB },
    { "bool", T_KW_TYP_BOOL },
};

static bool l_isSpecial(char ch)
{
    switch(ch) {
        case '\0':
        case ':': case '(': case ')':
        case ',': case ';':
            return true;
        default:
            return false;
    }
}

static const char *l_tokenize(const char *source, struct Tok *token, size_t *line)
{
    assert(source);
    assert(token);

has_spaces:
    if (isspace(source[0])) {
        if (source[0] == '\n' && line) ++(*line);
        ++source;
        goto has_spaces;
    }
    if (source[0] == '#') {
        while (source[0] && source[0] != '\n')
            ++source;
        if (source[0] == '\n') {
            ++source;
            if (line) ++(*line);
        }
        goto has_spaces;
    }

    token->line = line ? *line : 0;

    if (l_isSpecial(source[0])) {
        token->type = (enum TokType) source[0];
        token->origin = source;
        token->size = 1;
        return source+1;
    } else {
        token->origin = source;
        while (!l_isSpecial(source[0]) && !isspace(source[0])) ++source;
        token->size = source - token->origin;
        token->type = T_NAME;
        for (size_t i = 0; i < sizeof(l_keywords) / sizeof(l_keywords[0]); ++i) {
            if (strlen(l_keywords[i].kw) != token->size)
                continue;
            if (strncmp(l_keywords[i].kw, token->origin, token->size) != 0)
                continue;
            token->type = l_keywords[i].type;
        }
        return source;
    }
}

static bool l_synError(const struct Tok *tok, struct PAN *pan, const char *msg)
{
    pan->logger(
        "%sSyntax error @ line %zu, token `%.*s`: %s%s",
        (pan->color ? ESC_RED : ""), tok->line, tok->size, tok->origin,
        msg, (pan->color ? ESC_RST : "")
    );
    return false;
}


static char *l_dupToken(struct Tok *tok) 
{
    return strndup(tok->origin, tok->size);
}

bool pan_loadDefs(struct PAN *pan, const char *defs)
{
    assert(pan);
    assert(defs);

    struct Tok punct;

    size_t line = 1;
    while (1) {
        struct Tok defSide;
        defs = l_tokenize(defs, &defSide, &line);
        if (defSide.type == T_EOF)
            break;
        if (defSide.type != T_KW_CLIENT && defSide.type != T_KW_SERVER) 
            return l_synError(&defSide, pan, "Expected `client` or `server`");

        struct Tok defPrefix;
        defs = l_tokenize(defs, &defPrefix, &line);
        if (defPrefix.type < T_NAME) // after T_NAME are names
            return l_synError(&defPrefix, pan, "Message prefix name expected");
        if (defPrefix.size > 8)
            return l_synError(&defPrefix, pan, "Prefixes are no longer than 8 chars");

        defs = l_tokenize(defs, &punct, &line);
        if (punct.type != ':')
            return l_synError(&defPrefix, pan, "Expected `:` after prefix name");

        struct Tok defType;
        defs = l_tokenize(defs, &defType, &line);
        if (defType.type < T_NAME)
            return l_synError(&defType, pan, "Message type name expected");
        if (defType.size > 8)
            return l_synError(&defPrefix, pan, "Message types are no longer than 8 chars");

        defs = l_tokenize(defs, &punct, &line);
        if (punct.type != '(')
            return l_synError(&defPrefix, pan, "Expected `(` to open argument list");

        struct PAN_Arg *arg = NULL, **endptr = &arg;
        while (1) {
            struct Tok typeName;
            defs = l_tokenize(defs, &typeName, &line);
            if (typeName.type == ')')
                break;
            if (typeName.type < KW_MIN_TYPE) {
                l_destroyArg(arg);
                return l_synError(&typeName, pan, "Expected argument type name");
            }
            struct Tok tok;
            char *name = NULL;
            defs = l_tokenize(defs, &tok, &line);
            if (tok.type >= T_NAME) {
                name = strndup(tok.origin, tok.size);
                defs = l_tokenize(defs, &tok, &line);
            }

            struct PAN_Arg *newOne = calloc(1, sizeof(struct PAN_Arg));
            assert(newOne);
            newOne->type = (enum PAN_Type) ((int) typeName.type - KW_MIN_TYPE);
            newOne->name = name;
            *endptr = newOne;
            endptr = &newOne->next;

            if (tok.type == ')')
                break;
            if (tok.type == ',')
                continue;

            l_destroyArg(arg);
            return l_synError(&tok, pan, "Unexpected thing after argument");
        }

        struct PAN_MsgType *type = calloc(1, sizeof(struct PAN_MsgType));
        type->side = defSide.type == T_KW_SERVER ? PAN_SERVER : PAN_CLIENT;
        type->firstArg = arg;
        type->prefix = l_dupToken(&defPrefix);
        type->type = l_dupToken(&defType);
        type->prev = pan->msgTypes;
        pan->msgTypes = type;

        defs = l_tokenize(defs, &punct, &line);
        if (punct.type != ';')
            return l_synError(&defPrefix, pan, "Expected `;` after message definition");

    }

    return true;
}

bool pan_loadDefsFromFile(struct PAN *pan, const char *path)
{
    assert(pan != NULL);
    assert(path != NULL);

    FILE *f = fopen(path, "r");
    if (!f) return false;

    fseek(f, 0, SEEK_END); // those may fail, but skip them for now
    size_t size = ftell(f); // FIXME: handle their failiures
    char *buf = malloc(size + 1);
    if (!buf) return false;
    fseek(f, 0, SEEK_SET);
    fread(buf, 1, size, f);
    fclose(f);
    buf[size] = '\0';

    bool res = pan_loadDefs(pan, buf);
    free(buf);
    if (!res)
        errno = EINVAL;
    return res;
}

//==============================================================================
// Finding correct message type
//==============================================================================

struct PAN_MsgType *pan_find(struct PAN *pan, enum PAN_Side side, const char *pref, const char *type)
{
    struct PAN_MsgType *msg = pan->msgTypes;
    while (msg) {
        if (msg->side == side && strcmp(pref, msg->prefix) == 0 && strcmp(type, msg->type) == 0)
            return msg;
        msg = msg->prev;
    }
    return NULL;
}

struct PAN_MsgType *pan_binMatch(struct PAN *pan, enum PAN_Side side, const void *msg, size_t len)
{
    if (len < PAN_BINMSG_TYPE_LEN + PAN_BINMSG_PREF_LEN) return NULL;

    const char *cptr = (const char*) msg;
    char pbuf[PAN_BINMSG_PREF_LEN + 1] = {0}, tbuf[PAN_BINMSG_TYPE_LEN + 1] = {0};
    memcpy(pbuf, cptr, PAN_BINMSG_PREF_LEN);
    memcpy(tbuf, cptr + PAN_BINMSG_PREF_LEN, PAN_BINMSG_TYPE_LEN);

    return pan_find(pan, side, pbuf, tbuf);
}

//==============================================================================
// Dumping
//==============================================================================

struct DumpBuf {
    char *buf;
    size_t used, capacity;
    bool color;
};

static void l_db_killColor(struct DumpBuf *db)
{
    size_t wp = 0;
    bool killing = false;
    for (size_t i = 0; i < db->used; ++i) {
        if (db->buf[i] == '\x1b') killing = true;
        if (!killing) db->buf[wp++] = db->buf[i];
        if (db->buf[i] == 'm') killing = false;
    }
    db->buf[wp] = '\0';
    db->used = wp;
}

static void l_db_printf(struct DumpBuf *db, const char *fmt, ...)
{
    va_list args, copy;
    va_start(args, fmt);
    va_copy(copy, args);
    size_t size = vsnprintf(NULL, 0, fmt, copy) + 1;
    if (db->capacity < db->used + size) {
        db->capacity = MAX(size, db->capacity * 2 + 32);
        char *newSpace = realloc(db->buf, db->capacity);
        assert(newSpace);
        db->buf = newSpace;
    }
    db->used += vsnprintf(db->buf + db->used, size, fmt, args);
    va_end(copy);
    va_end(args);
}

static void l_db_hexdump(struct DumpBuf *db, const uint8_t *data, size_t from, size_t len) {
    l_db_printf(db, "    "); // tab
    for (size_t i = 0; i < from % PAN_HEXDUMP_WIDTH; ++i)
        l_db_printf(db, ESC_GRY ".. " ESC_RST);
    for (size_t i = 0; i < len; ++i) {
        l_db_printf(db, ESC_YLW "%02x " ESC_RST, data[from + i]);
        if ((from + i + 1) % PAN_HEXDUMP_WIDTH == 0 && i != len-1)
            l_db_printf(db, "\n    ");
    }
    if ((from + len) % PAN_HEXDUMP_WIDTH)
        for (size_t i = 0; i < PAN_HEXDUMP_WIDTH - (from + len) % PAN_HEXDUMP_WIDTH; ++i)
            l_db_printf(db, ESC_GRY ".. " ESC_RST);
}

typedef size_t (*BinDumpFunc)(struct DumpBuf *dbuf, const uint8_t *buf, size_t pos, size_t len, const char *label, bool longform);

#define SIMPLE_BDF(name, type, spec)\
    static size_t l_bdf_##name(struct DumpBuf *db, const uint8_t *buf, size_t pos, size_t len, const char *label, bool lform) {\
        type;\
        if (len < pos + sizeof(typeof(v))) {\
            if (lform) {\
                l_db_hexdump(db, buf, pos, len - pos);\
                l_db_printf(db, ESC_GRY " -- " ESC_CYN "%s " ESC_RST "%s " ESC_GRY "= " ESC_RED "[x] Message was cut\n" ESC_RST, #name, label);\
            } else {\
                l_db_printf(db, "%s " ESC_GRY "= " ESC_RED "[cut]" ESC_RST, label);\
            }\
            return len;\
        }\
        memcpy(&v, buf + pos, sizeof(typeof(v)));\
        if (lform) {\
            l_db_hexdump(db, buf, pos, sizeof(typeof(v)));\
            l_db_printf(db, ESC_GRY " -- " ESC_CYN "%s " ESC_RST "%s " ESC_GRY "= " ESC_YLW spec ESC_RST "\n", #name, label, v);\
        } else {\
            l_db_printf(db, "%s " ESC_GRY "= " ESC_YLW spec ESC_RST, label, v);\
        }\
        return pos + sizeof(typeof(v));\
    }

SIMPLE_BDF(id, uint32_t v, "%u");
SIMPLE_BDF(char64, char v[8], "%.8s");
SIMPLE_BDF(int8, int8_t v, "%d");
SIMPLE_BDF(int16, int16_t v, "%d");
SIMPLE_BDF(int32, int32_t v, "%d");
SIMPLE_BDF(int64, int64_t v, "%lld");
SIMPLE_BDF(float, float v, "%f");
SIMPLE_BDF(double, double v, "%lf");
SIMPLE_BDF(bool, int8_t v, "%u");

static size_t l_bdf_bloblike(struct DumpBuf *db, const uint8_t *buf, size_t pos, size_t len, const char *label, bool lform, bool isBlob)
{
    const char *name = isBlob ? "blob" : "string";
    if (len < pos + 2) {
        if (lform) {
            l_db_hexdump(db, buf, pos, len - pos);
            l_db_printf(
                db, ESC_GRY " -- " ESC_CYN "%s " ESC_RST "%s " ESC_GRY
                "= " ESC_RED "[x] Message was cut\n" ESC_RST, name, label
            );
        } else {
            l_db_printf(db, "%s " ESC_GRY " = " ESC_RED "[cut]" ESC_RST, label);
        }
        return len;
    }
    uint16_t strSize;
    memcpy(&strSize, buf + pos, 2);
    if (len < pos + 2 + strSize) {
        if (lform) {
            l_db_hexdump(db, buf, pos, len - pos);
            l_db_printf(
                db, ESC_GRY " -- " ESC_CYN "%s " ESC_RST "%s " ESC_GRY
                "of length " ESC_YLW "%d " ESC_GRY "= " ESC_RED "[x] Message was cut\n"
                ESC_RST, name, label, strSize
            );
        } else {
            l_db_printf(db, "%s" ESC_GRY " = " ESC_RED "[cut]" ESC_RST, label);
        }
        return len;
    }

    if (lform) {
        l_db_hexdump(db, buf, pos, 2 + strSize);
        if (isBlob) {
            l_db_printf(
                db, ESC_GRY " -- " ESC_CYN "blob " ESC_RST "%s " ESC_GRY
                "of length " ESC_YLW "%d " ESC_RST "\n"
                ESC_RST, label, strSize 
            );
        } else {
            l_db_printf(
                db, ESC_GRY " -- " ESC_CYN "string " ESC_RST "%s " ESC_GRY
                "of length " ESC_YLW "%d " ESC_GRY "= " ESC_GRN "`%.*s`\n"
                ESC_RST, label, strSize, strSize, buf + pos + 2
            );
        }
    } else {
        l_db_printf(db, "%s " ESC_GRY "= " ESC_RST, label);
        if (isBlob)
            l_db_printf(db, "blob[" ESC_YLW "%u" ESC_RST "]", strSize);
        else {
            uint16_t pl = strSize;
            if (pl > 32) pl = 32;
            l_db_printf(db, ESC_GRN "`%.*s%s" ESC_RST, pl, buf + pos + 2, (pl == strSize ? "`" : "`..."));
        }
    }
    return pos + 2 + strSize;
}

static size_t l_bdf_string(struct DumpBuf *db, const uint8_t *buf, size_t pos, size_t len, const char *label, bool lform)
{
   return l_bdf_bloblike(db, buf, pos, len, label, lform, false);
}

static size_t l_bdf_blob(struct DumpBuf *db, const uint8_t *buf, size_t pos, size_t len, const char *label, bool lform)
{
   return l_bdf_bloblike(db, buf, pos, len, label, lform, true);
}

#undef SIMPLE_BDF

BinDumpFunc bdf[] = {
    [PAN_ID] = l_bdf_id,
    [PAN_CHAR64] = l_bdf_char64,
    [PAN_INT8] = l_bdf_int8,
    [PAN_INT16] = l_bdf_int16,
    [PAN_INT32] = l_bdf_int32,
    [PAN_INT64] = l_bdf_int64,
    [PAN_FLOAT] = l_bdf_float,
    [PAN_DOUBLE] = l_bdf_double,
    [PAN_STRING] = l_bdf_string,
    [PAN_BLOB] = l_bdf_blob,
    [PAN_BOOL] = l_bdf_bool,
};

struct __attribute__((packed)) BinMsgHeader {
    char pref[PAN_BINMSG_PREF_LEN];
    char type[PAN_BINMSG_PREF_LEN];
    uint32_t seq;
    uint16_t bodyLen;
    uint16_t flags;
};

size_t pan_binDump(struct PAN *pan, enum PAN_Side side, const void *msg, size_t len)
{
    const uint8_t *cptr = (const uint8_t*) msg;

    struct DumpBuf db = {0};
    if (len < sizeof(struct BinMsgHeader)) {
        l_db_printf(&db, ESC_RED "partial message\n" ESC_RST);
        l_db_hexdump(&db, cptr, 0, len);
        l_db_printf(&db, ESC_GRY " -- " ESC_RED "[x] Message was cut before header ends\n" ESC_RST);
        goto end;
    }

    struct BinMsgHeader header;
    memcpy(&header, cptr, sizeof(header));
    l_db_printf(
        &db, ESC_BLU "%.8s " ESC_GRN "%s" ESC_GRY ":" ESC_RST "%.8s " ESC_YLW
        "#%zu " ESC_GRY "with " ESC_YLW "%zu " ESC_GRY "bytes of args\n" ESC_RST,
        side == PAN_CLIENT ? "client" : "server", header.pref,
        header.type, header.seq, header.bodyLen
    );

    l_db_hexdump(&db, cptr, offsetof(struct BinMsgHeader, pref), PAN_BINMSG_PREF_LEN);
    l_db_printf(&db, ESC_GRY " -- " ESC_RST "prefix " ESC_GRY "= " ESC_GRN "`%.8s`\n" ESC_RST, header.pref);
    l_db_hexdump(&db, cptr, offsetof(struct BinMsgHeader, type), PAN_BINMSG_TYPE_LEN);
    l_db_printf(&db, ESC_GRY " -- " ESC_RST "type " ESC_GRY "= " ESC_GRN "`%.8s`\n" ESC_RST, header.type);
    l_db_hexdump(&db, cptr, offsetof(struct BinMsgHeader, seq), 4);
    l_db_printf(&db, ESC_GRY " -- " ESC_RST "sequence number " ESC_GRY "= " ESC_YLW "%u\n" ESC_RST, header.seq);
    l_db_hexdump(&db, cptr, offsetof(struct BinMsgHeader, bodyLen), 2);
    l_db_printf(&db, ESC_GRY " -- " ESC_RST "body length " ESC_GRY "= " ESC_YLW "%u\n" ESC_RST, header.bodyLen);
    l_db_hexdump(&db, cptr, offsetof(struct BinMsgHeader, flags), 2);
    l_db_printf(&db, ESC_GRY " -- " ESC_RST "flags " ESC_GRY "= " ESC_YLW "%u\n" ESC_RST, header.flags);


    size_t pos = sizeof(struct BinMsgHeader);

    struct PAN_MsgType *type = pan_binMatch(pan, side, msg, len);

    if (!type) {
        size_t blen = header.bodyLen;
        if (pos + blen > len) blen = len - pos;
        l_db_hexdump(&db, cptr, pos, header.bodyLen);
        l_db_printf(&db, " -- <unknown body type>\n");
        if (header.bodyLen != blen) {
            l_db_printf(
                &db, "    /!\\ Warning: Body cuts before specified body length (%zu), %zu bytes missing\n",
                header.bodyLen, header.bodyLen - blen
            );
        }
        pos += blen;
        goto end;
    }

    struct PAN_Arg *arg = type->firstArg;
    while (arg != NULL) {
        pos = bdf[arg->type](&db, cptr, pos, len, arg->name ? arg->name : "<no name>", true);
        arg = arg->next;
    }
    size_t realBodyLen = pos - sizeof(struct BinMsgHeader);

    if (realBodyLen < header.bodyLen) {
        l_db_hexdump(&db, cptr, pos, header.bodyLen - realBodyLen);
        l_db_printf(&db, " -- /!\\ extra data\n");
    }

    if (realBodyLen != header.bodyLen) {
        l_db_printf(&db, "    /!\\ Warning: Declared message body length (%zu) != real body length (%zu)\n", header.bodyLen, realBodyLen);
    }

end:
    if (!pan->color)
        l_db_killColor(&db);
    pan->logger("%s", db.buf);
    free(db.buf);
    return pos;
}

size_t pan_binDump_short(struct PAN *pan, enum PAN_Side side, const void *msg, size_t len)
{
    const uint8_t *cptr = (const uint8_t*) msg;

    struct DumpBuf db = {0};
    if (len < sizeof(struct BinMsgHeader)) {
        l_db_printf(&db, ESC_RED "partial message" ESC_RST);
        goto end;
    }

    struct BinMsgHeader header;
    memcpy(&header, cptr, sizeof(header));
    l_db_printf(
        &db, ESC_BLU "%.8s " ESC_GRN "%s" ESC_GRY ":" ESC_RST "%.8s " ESC_YLW
        "#%zu " ESC_GRY "len = " ESC_YLW "%zu " ESC_GRY "(" ESC_RST,
        side == PAN_CLIENT ? "client" : "server", header.pref,
        header.type, header.seq, header.bodyLen
    );

    size_t pos = sizeof(struct BinMsgHeader);
    struct PAN_MsgType *type = pan_binMatch(pan, side, msg, len);

    if (!type) {
        l_db_printf(&db, ESC_RED "unknown type" ESC_GRY ")");
        goto end;
    }

    struct PAN_Arg *arg = type->firstArg;
    while (arg != NULL) {
        pos = bdf[arg->type](&db, cptr, pos, len, arg->name ? arg->name : "<no name>", false);
        if (arg->next)
            l_db_printf(&db, ESC_GRY ", " ESC_RST);
        arg = arg->next;
    }

    l_db_printf(&db, ESC_GRY ")");

end:
    if (!pan->color)
        l_db_killColor(&db);
    pan->logger("%s", db.buf);
    free(db.buf);
    return pos;

}


//==============================================================================
// Header generation
//==============================================================================

void l_makeIdentifier(char buf[8])
{
    for (size_t i = 0; i < 8 && buf[i] != '\0'; ++i)
        if (!isalnum(buf[i]) || (i == 0 && !isalpha(buf[i])))
            buf[i] = '_';
}

const char *l_mapTypeToC(enum PAN_Type t) {
    switch(t) {
        case PAN_ID: return "PAN_GH_ID";
        case PAN_CHAR64: return "PAN_GH_CHAR64";
        case PAN_INT8: return "int8_t";
        case PAN_INT16: return "int16_t";
        case PAN_INT32: return "int32_t";
        case PAN_INT64: return "int64_t";
        case PAN_FLOAT: return "float";
        case PAN_DOUBLE: return "double";
        case PAN_STRING: return "PAN_GH_SLICE";
        case PAN_BLOB: return "PAN_GH_SLICE";
        case PAN_BOOL: return "bool";
    }
}

void pan_generateHeader(struct PAN *pan, FILE *output, const char *incpath)
{
    fputs("// GENERATED FILE -- DO NOT EDIT\n", output);
    fputs("// Generated by libpan\n", output);
    fputs("// Contains:\n", output);
    for (struct PAN_MsgType *m = pan->msgTypes; m != NULL; m = m->prev)
        fprintf(output, "//  - %s %s:%s(...)\n", m->side == PAN_CLIENT ? "client" : "server", m->prefix, m->type);

    fputs("\n", output);
    fputs("#pragma once\n", output);
    fprintf(output, "#include \"%s\"\n", incpath);
    fputs("\n", output);

    fputs("PAN_GH_DEFS_BEGIN\n", output);
    fputs("\n", output);

    for (struct PAN_MsgType *m = pan->msgTypes; m != NULL; m = m->prev) {
        char pbuf[9] = {0}, tbuf[9] = {0};
        strncpy(pbuf, m->prefix, 8);
        strncpy(tbuf, m->type, 8);
        l_makeIdentifier(pbuf);
        l_makeIdentifier(tbuf);

        const char *loc = m->side == PAN_CLIENT ? "PAN_GH_CLIENT" : "PAN_GH_SERVER";

        fprintf(output, "PAN_GH_MSG(%s, %s, %s, \"%s\", \"%s\",\n", loc, pbuf, tbuf, m->prefix, m->type);
        for (struct PAN_Arg *a = m->firstArg; a != NULL; a = a->next)
            fprintf(output, "    %-16s %s;\n", l_mapTypeToC(a->type), a->name);
        fprintf(output, ")\n");

        fprintf(output, "PAN_GH_DECODE(%s, %s, %s, \"%s\", \"%s\",\n", loc, pbuf, tbuf, m->prefix, m->type);
        for (struct PAN_Arg *a = m->firstArg; a != NULL; a = a->next) {
            if (a->type == PAN_BLOB || a->type == PAN_STRING) {
                fprintf(output, "    uint16_t %s_len = 0;\n", a->name);
                fprintf(output, "    PAN_GH_READ(&%s_len, 2);\n", a->name);
                fprintf(output, "    PAN_GH_READ_SLICE(&self->%s, %s_len);\n", a->name, a->name);
            } else {
                fprintf(output, "    PAN_GH_READ(&self->%s, sizeof(self->%s));\n", a->name, a->name);
            }
        }
        fprintf(output, ")\n");

        fprintf(output, "PAN_GH_ENCODE(%s, %s, %s, \"%s\", \"%s\",\n", loc, pbuf, tbuf, m->prefix, m->type);
        fprintf(output, "    uint16_t len = 0;\n");
        for (struct PAN_Arg *a = m->firstArg; a != NULL; a = a->next) {
            if (a->type == PAN_BLOB || a->type == PAN_STRING)
                fprintf(output, "    len += 2 + PAN_GH_SLICE_LEN(&self->%s);\n", a->name);
            else
                fprintf(output, "    len += sizeof(self->%s);\n", a->name);
        }
        fprintf(output, "    PAN_GH_HEADER(\"%s\", \"%s\", len);\n", m->prefix, m->type);
        for (struct PAN_Arg *a = m->firstArg; a != NULL; a = a->next) {
            if (a->type == PAN_BLOB || a->type == PAN_STRING) {
                fprintf(output, "    uint16_t %s_len = PAN_GH_SLICE_LEN(&self->%s);\n", a->name, a->name);
                fprintf(output, "    PAN_GH_WRITE(&%s_len, 2);\n", a->name);
                fprintf(output, "    PAN_GH_WRITE_SLICE(&self->%s, %s_len);\n", a->name, a->name);
            } else {
                fprintf(output, "    PAN_GH_WRITE(&self->%s, sizeof(self->%s));\n", a->name, a->name);
            }
        }
        fprintf(output, ")\n");
        fputs("\n", output);
       
    }

    fputs("PAN_GH_DEFS_END\n", output);
    
}


