#pragma once

#include "modlib_mod.hpp"
#include "binmsg.hpp"
#include "Vec2.hpp"

#include <optional>
#include <string_view>
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

struct SpriteAsset {
    SpriteID id{};

    std::string file{};
    Recti source{};

    Vec2i size{};
    Vec2i origin{};
    Vec2i offset{};

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

