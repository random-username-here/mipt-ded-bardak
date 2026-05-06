#pragma once

#include "Timer.hpp"
#include "modlib_mod.hpp"
#include "binmsg.hpp"
#include "Vec2.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>
#include <vector>
#include <string>

namespace modlib {

struct Recti {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

using SpriteID = bmsg::Char64;
using SpriteIDHash = bmsg::Char64Hasher;

using AnimationID = bmsg::Char64;
using AnimationIDHash = bmsg::Char64Hasher;

using EventID = bmsg::Char64;
using EventIDHash = bmsg::Char64Hasher;

using Tick = Timer::Tick;

using AssetId = SpriteID;
inline const AssetId kInvalidAssetId{};

enum class AnimKind : uint8_t {
    Base = 0,
    Overlay,
    Particle,
};

enum class Playback : uint8_t {
    Once = 0,
    Loop,
};

struct SpriteAsset {
    SpriteID id{};

    std::string file{};
    Recti source{};

    Vec2i size{};
    Vec2i origin{};
    Vec2i offset{};

    int z = 0;
};

struct AnimationFrame {
    SpriteID sprite{};
    Tick duration = 0;
};

struct AnimationAsset {
    AnimationID id{};
    std::vector<AnimationFrame> frames{};

    Playback playback{Playback::Once};
    AnimKind kind{AnimKind::Base};

    int priority = 0;
    int z = 0;
};

struct VisualBinding {
    EventID event_id{};

    SpriteID sprite{};
    AnimationID animation{};
};

using SpriteVisitor    = std::function<void(const SpriteAsset &)>;
using AnimationVisitor = std::function<void(const AnimationAsset &)>;
using BindingVisitor   = std::function<void(const VisualBinding &)>;

class AssetManager : public Mod {
public:
    virtual bool registerSprite(const SpriteAsset &sprite) = 0;
    virtual bool registerAnimation(const AnimationAsset &animation) = 0;
    virtual bool registerBinding(const VisualBinding &binding) = 0;

    virtual std::optional<SpriteAsset> sprite(SpriteID id) const = 0;
    virtual std::optional<AnimationAsset> animation(AnimationID id) const = 0;
    virtual std::optional<VisualBinding> binding(EventID event_id) const = 0;

    virtual std::string_view spriteBytes(SpriteID id) const = 0;

    virtual void forEachSprite(const SpriteVisitor &visitor) const = 0;
    virtual void forEachAnimation(const AnimationVisitor &visitor) const = 0;
    virtual void forEachBinding(const BindingVisitor &visitor) const = 0;

    ~AssetManager() override = default;
};

} // namespace modlib

