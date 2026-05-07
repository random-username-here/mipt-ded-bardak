#pragma once

#include "modlib_mod.hpp"
#include "binmsg.hpp"
#include "Vec2.hpp"

#include <optional>
#include <string_view>
#include <string>

namespace modlib {

struct Rectf {
    float x = 0;
    float y = 0;
    float w = 0;
    float h = 0;
};

using SpriteID = bmsg::Char64;
using SpriteIDHash = bmsg::Char64Hasher;

struct SpriteAsset {
    SpriteID id{};
    std::string file{};
    std::string_view raw_bytes{};

    Rectf clip{};
    Vec2f size{};
    Vec2f origin{};
    Vec2f offset{};
};

class AssetManager : public Mod {
public:
    ~AssetManager() override = default;

    virtual bool registerSprite(SpriteAsset sprite) = 0;

	virtual std::optional<modlib::SpriteAsset> sprite(modlib::SpriteID id  ) const = 0;
	virtual std::optional<std::string_view>    bytes (modlib::SpriteID id  ) const = 0;
	virtual std::optional<std::string_view>    bytes (std::string_view file) const = 0;
};

} // namespace modlib

