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
    Rectf source{};

    Vec2f size{};
    Vec2f origin{};
    Vec2f offset{};

    int z = 0;
};

class AssetManager : public Mod {
public:
    virtual bool registerSprite(const SpriteAsset &sprite) = 0;
	virtual std::optional<modlib::SpriteAsset> sprite(modlib::SpriteID id) const = 0;
    virtual std::string_view spriteBytes(SpriteID id) const = 0;

    ~AssetManager() override = default;
};

} // namespace modlib

