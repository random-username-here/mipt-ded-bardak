/**
 * \file
 * \brief Macros for libPan's generated headers
 * \author Didyk Ivan
 *
 * Message structs are in namespace `bmsg`, with names like `CL_tank_shoot`,
 * and `static std::optional<...> decode(bmsg::RawMessage)` and 
 * `void encode(std::ostream&)`. Blobs are stored as `std::string_view`-s
 * (that means original data must live long as long as message is being used).
 *
 * Also note what created messages do not have a valid header, only decoded ones do.
 * Header is generated when message is encoded.
 *
 */
#pragma once

#include "binmsg.hpp"
#include <ostream>
#include <optional>
#include <string_view>
#include <cstring>
#include <cstdint>

#define PAN_GH_DEFS_BEGIN namespace bmsg {
#define PAN_GH_DEFS_END   };

#define PAN_GH_CLIENT CL
#define PAN_GH_SERVER SV

#define PAN_GH_ID     Id
#define PAN_GH_CHAR64 Char64
#define PAN_GH_SLICE  std::string_view

#define PAN_GH_NAME(loc, pref, type) loc##_##pref##_##type

#define PAN_GH_MSG(loc, ipref, itype, spref, stype, ...)\
    struct PAN_GH_NAME(loc, ipref, itype) {\
        __VA_ARGS__\
        Header header;\
        \
        static std::optional<PAN_GH_NAME(loc, ipref, itype)> decode(RawMessage msg);\
        void encode(std::ostream &str, Id id, uint16_t flags) const;\
    };

#define PAN_GH_DECODE_I(type, ...)\
    inline std::optional<type> type::decode(RawMessage _msg) {\
        if (!_msg.isCorrect()) return std::nullopt;\
        type _obj;\
        type *self = &_obj;\
        size_t _pos = 0;\
        self->header = *_msg.header();\
        __VA_ARGS__; /* contains PAN_GH_READ() */\
        return _obj;\
    }

#define PAN_GH_READ(buf, count)\
    do {\
        if (_pos + (count) < _msg.header()->len)\
            std::memcpy((buf), (const char*) _msg.bodyPtr() + _pos, (count));\
        else\
            return std::nullopt;\
    } while (0)

#define PAN_GH_READ_SLICE(ref, count)\
    do {\
        if (_pos + (count) < _msg.header()->len)\
            *(ref) = std::string_view((const char*) _msg.bodyPtr() + _pos, (count));\
        else\
            return std::nullopt;\
    } while (0)

#define PAN_GH_DECODE(loc, ipref, itype, spref, stype, ...)\
    PAN_GH_DECODE_I(PAN_GH_NAME(loc, ipref, itype), __VA_ARGS__)

#define PAN_GH_ENCODE_I(type, spref, stype, ...)\
    inline void type::encode(std::ostream &_os, Id _id, uint16_t _flags) const {\
        const type* self = this;\
        __VA_ARGS__\
    }

#define PAN_GH_HEADER(spref, stype, size) do {\
        Header _hdr = { .pref = spref, .type = stype, .id = _id, .len = (size), .flags = _flags };\
        _os.write((const char*) &_hdr, sizeof(_hdr));\
    } while (0)
#define PAN_GH_WRITE(buf, count) _os.write((const char*) buf, count)
#define PAN_GH_SLICE_LEN(ref) ((ref)->size())
#define PAN_GH_WRITE_SLICE(ref, _) _os.write((ref)->data(), (ref)->size())

#define PAN_GH_ENCODE(loc, ipref, itype, spref, stype, ...)\
    PAN_GH_ENCODE_I(PAN_GH_NAME(loc, ipref, itype), spref, stype, __VA_ARGS__)
