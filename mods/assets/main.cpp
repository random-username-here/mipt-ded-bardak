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
    std::unordered_map<modlib::AnimationID, modlib::AnimationAsset, modlib::AnimationIDHash> m_animations;
    std::unordered_map<modlib::EventID, modlib::VisualBinding, modlib::EventIDHash> m_bindings;
    std::unordered_map<std::string_view, std::string> m_fileBytes;

public:
    std::string_view id() const override { return "neilor.bardak.asset_manager"; }
    std::string_view brief() const override { return "Stores assets once and serves read-only sprite/animation bindings"; }
    ModVersion version() const override { return ModVersion(0, 3, 0); }

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

    bool registerAnimation(const modlib::AnimationAsset &animation) override {
        const auto animationId = animation.id.as_u64;
        if (animationId == 0 || m_animations.count(animationId) != 0) {
            return false;
        }
        m_animations.emplace(animationId, animation);
        return true;
    }

    bool registerBinding(const modlib::VisualBinding &binding) override {
        if (binding.event_id.as_u64 == 0) {
            return false;
        }

        if (binding.sprite.as_u64 != 0 && m_sprites.count(binding.sprite.as_u64) == 0) {
            return false;
        }
        if (binding.animation.as_u64 != 0 && m_animations.count(binding.animation.as_u64) == 0) {
            return false;
        }

		auto [_, res] = m_bindings.try_emplace(binding.event_id, binding);
		return res;
    }

    std::optional<modlib::SpriteAsset> sprite(modlib::SpriteID id) const override {
        auto it = m_sprites.find(id);
        if (it == m_sprites.end()) {
            return std::nullopt;
        }
        return it->second.asset;
    }

    std::optional<modlib::AnimationAsset> animation(modlib::AnimationID id) const override {
        auto it = m_animations.find(id);
        if (it == m_animations.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::optional<modlib::VisualBinding> binding(modlib::EventID id) const override {
        auto it = m_bindings.find(id);
        if (it == m_bindings.end()) {
            return std::nullopt;
        }
        return it->second;
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

    void forEachSprite(const modlib::SpriteVisitor &visitor) const override {
        for (const auto &entry : m_sprites) {
            visitor(entry.second.asset);
        }
    }

    void forEachAnimation(const modlib::AnimationVisitor &visitor) const override {
        for (const auto &entry : m_animations) {
            visitor(entry.second);
        }
    }

    void forEachBinding(const modlib::BindingVisitor &visitor) const override {
        for (const auto &entry : m_bindings) {
            visitor(entry.second);
        }
    }
};

extern "C" Mod *modlib_create(ModManager *) {
    return new AssetManagerModule();
}

