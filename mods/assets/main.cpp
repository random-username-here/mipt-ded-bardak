#include "AssetManager.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>

namespace {

std::string loadFileBytes(std::string_view filePath) {
    std::ifstream input_file(std::filesystem::path(filePath), std::ios::binary);
    if (!input_file) {
        return {};
    }

    input_file.seekg(0, std::ios::end);
    const std::streamsize fileSize = input_file.tellg();
    if (fileSize <= 0) {
        return {};
    }
    input_file.seekg(0, std::ios::beg);

    std::string bytes;
    bytes.resize(static_cast<size_t>(fileSize));
    if (!input_file.read(bytes.data(), fileSize)) {
        return {};
    }
    return bytes;
}

bool isPNG(std::string_view raw_bytes)
{
	constexpr unsigned char PNG_signature[] = {
		0x89, 0x50, 0x4E, 0x47,
		0x0D, 0x0A, 0x1A, 0x0A
	};

	if (raw_bytes.size() < sizeof(PNG_signature)) {
		return false;
	}

	const auto *data = reinterpret_cast<const unsigned char *>(raw_bytes.data());
	for (std::size_t i = 0; i < sizeof(PNG_signature); ++i) {
		if (data[i] != PNG_signature[i]) {
			return false;
		}
	}

	return true;
}

} // namespace

class AssetManagerModule final : public modlib::AssetManager {
private:
	using SpritesMap = std::unordered_map<modlib::SpriteID, modlib::SpriteAsset, modlib::SpriteIDHash>;
	using BytesMap = std::unordered_map<std::string_view, std::string>;

    SpritesMap m_sprites;
    BytesMap m_bytes;

public:
    std::string_view id() const override { return "neilor.bardak.asset_manager"; }
    std::string_view brief() const override { return "Manager of raw resources, e.g. sprites"; }
    ModVersion version() const override { return ModVersion(0, 1, 0); }

    bool registerSprite(modlib::SpriteAsset sprite) override {
        const auto sprite_id = sprite.id.as_u64;
        if (sprite_id == 0 || m_sprites.count(sprite_id) != 0) {
            return false;
        }

        if (sprite.file.empty()) {
            return false;
        }

        auto bytes_it = m_bytes.find(sprite.file);

        if (bytes_it == m_bytes.end()) {
			std::string raw_bytes = loadFileBytes(sprite.file);
			if (!isPNG(raw_bytes))
			{
				std::cerr << "Only PNG format for assets is supported (for compatibility with raylib)\n";
				return false;
			}

			bool ok;
			std::tie(bytes_it, ok) = m_bytes.try_emplace(sprite.file, std::move(raw_bytes));
            if (!ok || bytes_it->second.empty()) {
                return false;
            }
        }

        sprite.raw_bytes = bytes_it->second;
        m_sprites.emplace(sprite_id, std::move(sprite));
        return true;
    }

    std::optional<modlib::SpriteAsset> sprite(modlib::SpriteID id) const override {
        auto sprite_it = m_sprites.find(id);
        if (sprite_it == m_sprites.end()) {
            return std::nullopt;
        }
        
		return sprite_it->second;
    }

    std::optional<std::string_view> bytes(modlib::SpriteID id) const override {
		const auto sprite_it = m_sprites.find(id);
        if (sprite_it == m_sprites.end()) {
            return std::nullopt;
        }

		return bytes(sprite_it->second.file);
	}

    std::optional<std::string_view> bytes(std::string_view file) const override {
        const auto bytes_it = m_bytes.find(file);
        if (bytes_it == m_bytes.end()) {
            return std::nullopt;
        }

        return bytes_it->second;
    }
};

extern "C" Mod *modlib_create(ModManager *) {
    return new AssetManagerModule();
}
