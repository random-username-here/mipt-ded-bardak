#pragma once

#include "modlib_mod.hpp"
#include "binmsg.hpp"
#include "Vec2.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace modlib {

struct Rectf {
    float x = 0;
    float y = 0;
    float w = 0;
    float h = 0;
};

using AssetID     = bmsg::Char64;
using AssetIDHash = bmsg::Char64Hasher;

using SpriteID     = AssetID;
using SpriteIDHash = AssetIDHash;

using SoundID     = AssetID;
using SoundIDHash = AssetIDHash;

using MusicID     = AssetID;
using MusicIDHash = AssetIDHash;

struct SpriteAsset {
    SpriteID id{};
    std::string file{};
    std::string_view raw_bytes{};

    Rectf clip{};
    Vec2f size{};
    Vec2f origin{};
    Vec2f offset{};
};

struct SoundAsset {
    SoundID id{};
    std::string file{};
    std::string_view raw_bytes{};

    float volume       = 1.0f;
    float pitch        = 1.0f;
    float delaySeconds = 0.0f;
    int   priority     = 0;

    size_t maxPerTick = 1;  // 0 for unlimited

    // If non-empty, limit by this group instead of by id
    // e.g. slash/cut may both use group "attack"
    bmsg::Char64 group{};
};

struct MusicAsset {
    MusicID id{};
    std::string file{};
    std::string_view raw_bytes{};

    float volume = 0.35f;
    bool  loop   = true;
};

class AssetManager : public Mod {
public:
    ~AssetManager() override = default;

    virtual bool registerSprite(SpriteAsset sprite) = 0;
    virtual bool registerSound (SoundAsset  sound)  = 0;
    virtual bool registerMusic (MusicAsset  music)  = 0;

    virtual std::optional<SpriteAsset> sprite(SpriteID id) const = 0;
    virtual std::optional<SoundAsset>  sound (SoundID  id) const = 0;
    virtual std::optional<MusicAsset>  music (MusicID  id) const = 0;

    virtual std::optional<std::string_view> bytes(AssetID id)            const = 0;
    virtual std::optional<std::string_view> bytes(std::string_view file) const = 0;
};

} // namespace modlib
