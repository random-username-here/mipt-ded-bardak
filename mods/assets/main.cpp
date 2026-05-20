#include "AssetManager.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
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
    using SoundsMap  = std::unordered_map<modlib::SoundID,  modlib::SoundAsset,  modlib::SoundIDHash>;
    using MusicMap   = std::unordered_map<modlib::MusicID,  modlib::MusicAsset,  modlib::MusicIDHash>;
    using BytesMap   = std::unordered_map<std::string, std::string>;

    SpritesMap m_sprites;
    SoundsMap  m_sounds;
    MusicMap   m_music;
    BytesMap   m_bytes;

public:
    std::string_view id() const override { return "neilor.bardak.asset_manager"; }
    std::string_view brief() const override { return "Manager of raw resources, sprites, and sounds"; }
    ModVersion version() const override { return ModVersion(0, 2, 0); }

    bool registerSprite(modlib::SpriteAsset sprite) override {
        const auto sprite_id = sprite.id.as_u64;
        if (sprite_id == 0 || m_sprites.count(sprite.id) != 0) {
            return false;
        }

        if (sprite.file.empty()) {
            return false;
        }

        auto bytes_it = bytesForFile(sprite.file);
        if (bytes_it == m_bytes.end()) {
            return false;
        }

        if (!isPNG(bytes_it->second)) {
            std::cerr << "Only PNG format is supported for sprite assets: " << sprite.file << "\n";
            return false;
        }

        sprite.raw_bytes = bytes_it->second;
        m_sprites.emplace(sprite.id, std::move(sprite));
        return true;
    }

    bool registerSound(modlib::SoundAsset sound) override {
        const auto sound_id = sound.id.as_u64;
        if (sound_id == 0 || m_sounds.count(sound.id) != 0) {
            return false;
        }

        if (sound.file.empty()) {
            return false;
        }

        auto bytes_it = bytesForFile(sound.file);
        if (bytes_it == m_bytes.end()) {
            return false;
        }

        sound.raw_bytes = bytes_it->second;
        m_sounds.emplace(sound.id, std::move(sound));
        return true;
    }

    bool registerMusic(modlib::MusicAsset music) override {
        const auto music_id = music.id.as_u64;
        if (music_id == 0 || m_music.count(music.id) != 0) {
            return false;
        }

        if (music.file.empty()) {
            return false;
        }

        auto bytes_it = bytesForFile(music.file);
        if (bytes_it == m_bytes.end()) {
            return false;
        }

        music.raw_bytes = bytes_it->second;
        m_music.emplace(music.id, std::move(music));
        return true;
    }

    std::optional<modlib::SpriteAsset> sprite(modlib::SpriteID id) const override {
        auto sprite_it = m_sprites.find(id);
        if (sprite_it == m_sprites.end()) {
            return std::nullopt;
        }
        
		return sprite_it->second;
    }

    std::optional<modlib::SoundAsset> sound(modlib::SoundID id) const override {
        auto sound_it = m_sounds.find(id);
        if (sound_it == m_sounds.end()) {
            return std::nullopt;
        }

        return sound_it->second;
    }

    std::optional<modlib::MusicAsset> music(modlib::MusicID id) const override {
        auto music_it = m_music.find(id);
        if (music_it == m_music.end()) {
            return std::nullopt;
        }

        return music_it->second;
    }

    std::optional<std::string_view> bytes(modlib::AssetID id) const override {
        if (const auto sprite_it = m_sprites.find(id); sprite_it != m_sprites.end()) {
            return bytes(sprite_it->second.file);
        }

        if (const auto sound_it = m_sounds.find(id); sound_it != m_sounds.end()) {
            return bytes(sound_it->second.file);
        }

        if (const auto music_it = m_music.find(id); music_it != m_music.end()) {
            return bytes(music_it->second.file);
        }

        return std::nullopt;
    }

    std::optional<std::string_view> bytes(std::string_view file) const override {
        const auto bytes_it = m_bytes.find(std::string(file));
        if (bytes_it == m_bytes.end()) {
            return std::nullopt;
        }

        return bytes_it->second;
    }

private:
    BytesMap::iterator bytesForFile(const std::string &file) {
        auto bytes_it = m_bytes.find(file);
        if (bytes_it != m_bytes.end()) {
            return bytes_it;
        }

        std::string raw_bytes = loadFileBytes(file);
        if (raw_bytes.empty()) {
            std::cerr << "Failed to load asset file: " << file << "\n";
            return m_bytes.end();
        }

        bool ok = false;
        std::tie(bytes_it, ok) = m_bytes.try_emplace(file, std::move(raw_bytes));
        if (!ok) {
            return m_bytes.end();
        }

        return bytes_it;
    }
};

extern "C" Mod *modlib_create(ModManager *) {
    return new AssetManagerModule();
}
