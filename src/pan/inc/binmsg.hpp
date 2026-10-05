/**
 * \file
 * \brief top-level `bmsg` message format
 * \author Ivan Didyk
 * \date 2026-04-20
 *
 * This header is mainly for standardizing plugin API-s.
 */
#pragma once
#include <cassert>
#include <cstdint>
#include <string_view>
#include <cstring>

namespace bmsg {

/** 
 * \brief Short string of 8 characters
 */
union Char64 {
    uint64_t as_u64;
    char as_chars[8];

    constexpr Char64() : as_u64(0) {}
    constexpr Char64(uint64_t v) : as_u64(v) {}

    constexpr Char64(std::string_view s) : as_chars{0,0,0,0,0,0,0,0} { 
        if (s.size() > 8) { 
            throw "s.size() should be <= 8"; 
        }
        
        for (size_t i = 0; i < s.size(); ++i) {
            as_chars[i] = s[i]; 
        }
    }

    constexpr Char64(const char* c) : Char64(std::string_view(c)) {}

    constexpr size_t size() const {
        size_t s = 0;
        while (s < 8 && as_chars[s] != '\0') ++s;
        return s;
    }

    constexpr Char64 &operator=(const std::string_view &s) {
        *this = Char64(s);
        return *this;
    }

    constexpr operator uint64_t() const { return as_u64; }
    constexpr operator std::string_view() const { return std::string_view(as_chars, size()); }
};

struct Char64Hasher {
    size_t operator()(const bmsg::Char64& c) const {
        return std::hash<uint64_t>{}(c.as_u64);
    }
};

constexpr inline bool operator==(Char64 lhs, Char64 rhs) { return lhs.as_u64 == rhs.as_u64; }
constexpr inline bool operator!=(Char64 lhs, Char64 rhs) { return !(lhs == rhs); }

/** 
 * \brief Message ID type
 */
using Id = uint32_t;

struct Flags {
    uint16_t as_u16;

    Flags() :as_u16(0) {}
    constexpr Flags(uint16_t v) :as_u16(v) {}
    operator uint16_t() const { return as_u16; }

    bool has(Flags f) const { return (as_u16 | f.as_u16) == as_u16; }
    Flags operator|(Flags o) const { return Flags(o.as_u16 | as_u16); }
    Flags &operator|=(Flags o) { as_u16 |= o.as_u16; return *this; }
};

/// Disable buffering of this message, send it straight away
static const Flags NO_BUF = Flags(1);

/// Send this message as UDP
static const Flags USE_UDP = Flags(2);

/** 
 * \brief BMSG header structure
 * Flags are currently not being used by standard for anything.
 * Note on this not being packed: this is due to Char64 not being POD because
 * of convenience constructors. But this works fine, because this is packed by hand.
 */
struct Header {
    Char64 pref;
    Char64 type;
    Id id;
    uint16_t len;
    Flags flags;
};

/** View to some BMSG message with non-decoded header */
class RawMessage {
    std::string_view m_data;
public:
    RawMessage(std::string_view buf) :m_data(buf) {}

    std::string_view data() const { return m_data; }

    /**
     * \brief Obtain message header.
     * If message is cut such that it is shorter than header,
     * this will return nullptr. 
     */
    const Header *header() const {
        if (data().size() < sizeof(Header))
            return nullptr;
        return reinterpret_cast<const Header*>(data().data());
    }

    /** 
     * \brief Message body as string view. 
     * Will return nullptr stringview if message is not valid.
     */
    std::string_view body() const {
        if (!isCorrect())
            return std::string_view();
        return data().substr(sizeof(Header));
    }

    /** 
     * \brief Pointer to message body
     * Use this carefully, only if you checked body size beforehand.
     */
    const void *bodyPtr() const {
        return body().data();
    }

    /**
     * \brief Is message's length correct?
     * This does not care about contents of the body,
     * and for things like missing arguments.
     * Also, this allows 
     */
    bool isCorrect() const {
        auto head = header();
        if (!head) return false;
        return data().size() >= sizeof(Header) + head->len;
    }

    /**
     * \brief Data after this message
     */
    std::string_view tail() const {
        return m_data.substr(sizeof(Header) + header()->len);
    }
};

};
