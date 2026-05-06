#include "AssetManager.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <utility>

namespace {

std::string loadFileBytes(std::string_view filePath) {
    std::ifstream inputFile(std::filesystem::path(filePath), std::ios::binary);
    if (!inputFile) {
        return {};
    }

    inputFile.seekg(0, std::ios::end);
    const std::streamsize fileSize = inputFile.tellg();
    if (fileSize <= 0) {
        return {};
    }
    inputFile.seekg(0, std::ios::beg);

    std::string bytes;
    bytes.resize(static_cast<size_t>(fileSize));
    if (!inputFile.read(bytes.data(), fileSize)) {
        return {};
    }
    return bytes;
}

} // namespace

class AssetManagerModule final : public modlib::AssetManager {
private:
    struct StoredSprite {
        modlib::SpriteAsset asset{};
        std::string_view file{};
        std::string bytesKey{};
    };

    std::unordered_map<modlib::SpriteID, StoredSprite, modlib::SpriteIDHash> m_sprites;
    std::unordered_map<std::string_view, std::string> m_fileBytes;

public:
    std::string_view id() const override { return "neilor.bardak.asset_manager"; }
    std::string_view brief() const override { return "Manager of raw resources, e.g. sprites"; }
    ModVersion version() const override { return ModVersion(0, 1, 0); }

    bool registerSprite(const modlib::SpriteAsset &sprite) override {
        const auto spriteId = sprite.id.as_u64;
        if (spriteId == 0 || m_sprites.count(spriteId) != 0) {
            return false;
        }

        StoredSprite stored;
        stored.file = sprite.file;
        if (stored.file.empty()) {
            return false;
        }
        auto cachedBytesIt = m_fileBytes.find(stored.file);
        if (cachedBytesIt == m_fileBytes.end()) {
            std::string bytes = loadFileBytes(stored.file);
            if (bytes.empty()) {
                return false;
            }
            cachedBytesIt = m_fileBytes.emplace(stored.file, std::move(bytes)).first;
        }

        stored.asset = sprite;
        stored.bytesKey = cachedBytesIt->first;
        m_sprites.emplace(spriteId, std::move(stored));
        return true;
    }

    std::optional<modlib::SpriteAsset> sprite(modlib::SpriteID id) const override {
        auto it = m_sprites.find(id);
        if (it == m_sprites.end()) {
            return std::nullopt;
        }
        return it->second.asset;
    }

    std::string_view spriteBytes(modlib::SpriteID id) const override {
        const auto spriteIt = m_sprites.find(id.as_u64);
        if (spriteIt == m_sprites.end()) {
            return {};
        }

        const auto bytesIt = m_fileBytes.find(spriteIt->second.bytesKey);
        if (bytesIt == m_fileBytes.end()) {
            return {};
        }
        return bytesIt->second;
    }
};

extern "C" Mod *modlib_create(ModManager *) {
    return new AssetManagerModule();
}
