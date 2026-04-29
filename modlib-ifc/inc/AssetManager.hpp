#pragma once

#include "modlib_mod.hpp"
#include <cstdint>
#include <string_view>

namespace modlib {

using AssetId = uint32_t;

enum class AssetKind : uint8_t {
    Tile = 0,
    Unit = 1,
};

// Transformation metadata that visualization can apply on top of raw bytes.
// This is intentionally "lightweight": we avoid creating 360 rotated copies.
struct AssetTransform {
    // Rotation in milli-degrees (deg * 1000).
    // Example: 90 deg => 90000.
    int32_t rotationMilliDeg = 0;
};

// Asset manager "interface" for other server-side modules.
// Implemented by `mods/assets`.
class AssetManager : public Mod {
public:
    virtual AssetId addTexture(AssetKind kind, std::string_view filePath) = 0;

    // Get raw bytes. For transformed assets, bytes are expected to reference
    // the base texture (no image processing required server-side).
    virtual std::string_view getTextureBytes(AssetId id) const = 0;
    virtual AssetTransform getTransform(AssetId id) const = 0;

    // Create (and cache) a transformed "view" of `baseId` without duplicating bytes.
    virtual AssetId getTransformed(AssetId baseId, AssetTransform transform) = 0;

    virtual ~AssetManager() = default;
};

} // namespace modlib

