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

namespace modlib {

struct Recti {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

using VisualID = bmsg::Char64;
using SpriteID = bmsg::Char64;
using AnimationID = bmsg::Char64;
using BindingID = bmsg::Char64;
using AssetKeyKind = bmsg::Char64;
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

enum class VisualClass : uint8_t {
    Tile = 0,
    Sprite,
    Effect,
};

enum class Rotation90 : uint8_t {
    R0 = 0,
    R90,
    R180,
    R270,
};

struct VisualTransform {
    Rotation90 rotation{Rotation90::R0};
    bool flipX = false;
    bool flipY = false;
};

struct SpriteAsset {
    SpriteID id{};
    VisualClass visualClass{VisualClass::Sprite};

    std::string_view file{};
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
    VisualClass visualClass{VisualClass::Sprite};
    std::vector<AnimationFrame> frames{};

    Playback playback{Playback::Once};
    AnimKind kind{AnimKind::Base};

    int priority = 0;
    int z = 0;
};

struct AssetKey {
    AssetKeyKind kind{};
    uint64_t numeric = 0;
    bmsg::Char64 symbolic{};
};

struct VisualBinding {
    BindingID id{};
    AssetKey key{};

    SpriteID sprite{};
    AnimationID animation{};
    VisualTransform transform{};
};

using SpriteVisitor = std::function<void(const SpriteAsset &)>;
using AnimationVisitor = std::function<void(const AnimationAsset &)>;
using BindingVisitor = std::function<void(const VisualBinding &)>;

class AssetManager : public Mod {
public:
    virtual bool registerSprite(const SpriteAsset &sprite) = 0;
    virtual bool registerAnimation(const AnimationAsset &animation) = 0;

    virtual bool bind(const VisualBinding &binding) = 0;

    virtual std::optional<SpriteAsset> sprite(SpriteID id) const = 0;
    virtual std::optional<AnimationAsset> animation(AnimationID id) const = 0;
    virtual std::optional<SpriteAsset> tile(SpriteID id) const = 0;
    virtual std::optional<AnimationAsset> animatedSprite(AnimationID id) const = 0;
    virtual std::optional<AnimationAsset> animatedTile(AnimationID id) const = 0;
    virtual std::optional<SpriteAsset> effect(SpriteID id) const = 0;
    virtual std::optional<AnimationAsset> animatedEffect(AnimationID id) const = 0;

    // Raw atlas/sheet bytes loaded once during sprite registration.
    virtual std::string_view spriteBytes(SpriteID id) const = 0;

    virtual std::optional<VisualBinding> binding(const AssetKey &key) const = 0;
    virtual std::optional<VisualBinding> binding(BindingID id, const AssetKey &key) const = 0;
    virtual std::vector<VisualBinding> bindings(BindingID id) const = 0;

    virtual void forEachSprite(const SpriteVisitor &visitor) const = 0;
    virtual void forEachAnimation(const AnimationVisitor &visitor) const = 0;
    virtual void forEachBinding(const BindingVisitor &visitor) const = 0;

    ~AssetManager() override = default;
};

} // namespace modlib

