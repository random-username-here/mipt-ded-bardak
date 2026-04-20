/**
 * \file
 * \brief top-level `bmsg` message format
 * \author Ivan Didyk
 * \date 2026-04-20
 *
 * This header is mainly for standardizing plugin API-s.
 */
#pragma once
#include <cstdint>
#include <string_view>
#include <cstring>

namespace bmsg {

/** 
 * \brief Short string of 8 characters
 */
union Char64 {
    char as_chars[8];
    uint64_t as_u64;
};

inline bool operator==(Char64 c, std::string_view s) {
    if (s.size() > 8) return false;
    return strncmp(c.as_chars, s.data(), s.size()) == 0;
}
inline bool operator==(std::string_view s, Char64 c) { return c == s; }
inline bool operator!=(Char64 c, std::string_view s) { return !(c == s); }
inline bool operator!=(std::string_view s, Char64 c) { return !(c == s); }

/** 
 * \brief Message ID type
 */
using Id = uint32_t;

/** 
 * \brief BMSG header structure
 * Flags are currently not being used by standard for anything.
 */
struct __attribute__((packed)) Header {
    Char64 pref;
    Char64 type;
    Id id;
    uint16_t len;
    uint16_t flags;
};

/** View to some BMSG message with non-decoded header */
class RawMessage {
    std::string_view m_data;
public:
    RawMessage(std::string_view buf) :m_data(buf) {}

    std::string_view data() { return m_data; }

    /**
     * \brief Obtain message header.
     * If message is cut such that it is shorter than header,
     * this will return nullptr. 
     */
    const Header *header() {
        if (data().size() < sizeof(Header))
            return nullptr;
        return reinterpret_cast<const Header*>(data().data());
    }

    /** 
     * \brief Message body as string view. 
     * Will return nullptr stringview if message is not valid.
     */
    std::string_view body() {
        if (!isCorrect())
            return std::string_view();
        return data().substr(sizeof(Header));
    }

    /** 
     * \brief Pointer to message body
     * Use this carefully, only if you checked body size beforehand.
     */
    const void *bodyPtr() {
        return body().data();
    }

    /**
     * \brief Is message's length correct?
     * This does not care about contents of the body,
     * and for things like missing arguments.
     */
    bool isCorrect() {
        auto head = header();
        if (!head) return false;
        return data().size() == sizeof(Header) + head->len;
    }
};
};
