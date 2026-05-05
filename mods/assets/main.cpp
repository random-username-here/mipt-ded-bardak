#include "AssetManager.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

std::string loadFileBytes(const std::string &filePath) {
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
public:
    std::string_view id() const override { return "neilor.bardak.asset_manager"; }
    std::string_view brief() const override { return "Stores assets once and serves read-only sprite/animation bindings"; }
    modlib::ModVersion version() const override { return modlib::ModVersion(0, 3, 0); }

    bool registerSprite(const modlib::SpriteAsset &sprite) override {
        const auto spriteId = sprite.id.as_u64;
        if (spriteId == 0 || m_sprites.count(spriteId) != 0) {
            return false;
        }

        StoredSprite stored;
        stored.file = std::string(sprite.file);
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
        stored.asset.file = stored.file;
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

    bool bind(const modlib::VisualBinding &binding) override {
        if (binding.id.as_u64 == 0) {
            return false;
        }

        if (binding.sprite.as_u64 != 0 && m_sprites.count(binding.sprite.as_u64) == 0) {
            return false;
        }
        if (binding.animation.as_u64 != 0 && m_animations.count(binding.animation.as_u64) == 0) {
            return false;
        }

        const AssetKeyIdx keyIdx(binding.key);
        const BindingKeyIdx bindingKeyIdx(binding.id, binding.key);
        if (m_bindingByKey.count(keyIdx) != 0 || m_bindingByPair.count(bindingKeyIdx) != 0) {
            return false;
        }

        m_bindings.push_back(binding);
        m_bindingByKey.emplace(keyIdx, binding);
        m_bindingByPair.emplace(bindingKeyIdx, binding);
        m_bindingsById[binding.id.as_u64].push_back(binding);
        return true;
    }

    std::optional<modlib::SpriteAsset> sprite(modlib::SpriteID id) const override {
        auto it = m_sprites.find(id.as_u64);
        if (it == m_sprites.end()) {
            return std::nullopt;
        }

        modlib::SpriteAsset out = it->second.asset;
        out.file = it->second.file;
        return out;
    }

    std::optional<modlib::AnimationAsset> animation(modlib::AnimationID id) const override {
        auto it = m_animations.find(id.as_u64);
        if (it == m_animations.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::optional<modlib::SpriteAsset> tile(modlib::SpriteID id) const override {
        return spriteByClass(id, modlib::VisualClass::Tile);
    }

    std::optional<modlib::AnimationAsset> animatedSprite(modlib::AnimationID id) const override {
        return animationByClass(id, modlib::VisualClass::Sprite);
    }

    std::optional<modlib::AnimationAsset> animatedTile(modlib::AnimationID id) const override {
        return animationByClass(id, modlib::VisualClass::Tile);
    }

    std::optional<modlib::SpriteAsset> effect(modlib::SpriteID id) const override {
        return spriteByClass(id, modlib::VisualClass::Effect);
    }

    std::optional<modlib::AnimationAsset> animatedEffect(modlib::AnimationID id) const override {
        return animationByClass(id, modlib::VisualClass::Effect);
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

    std::optional<modlib::VisualBinding> binding(const modlib::AssetKey &key) const override {
        auto it = m_bindingByKey.find(AssetKeyIdx(key));
        if (it == m_bindingByKey.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::optional<modlib::VisualBinding> binding(modlib::BindingID id, const modlib::AssetKey &key) const override {
        auto it = m_bindingByPair.find(BindingKeyIdx(id, key));
        if (it == m_bindingByPair.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::vector<modlib::VisualBinding> bindings(modlib::BindingID id) const override {
        auto it = m_bindingsById.find(id.as_u64);
        if (it == m_bindingsById.end()) {
            return {};
        }
        return it->second;
    }

    void forEachSprite(const modlib::SpriteVisitor &visitor) const override {
        for (const auto &entry : m_sprites) {
            modlib::SpriteAsset out = entry.second.asset;
            out.file = entry.second.file;
            visitor(out);
        }
    }

    void forEachAnimation(const modlib::AnimationVisitor &visitor) const override {
        for (const auto &entry : m_animations) {
            visitor(entry.second);
        }
    }

    void forEachBinding(const modlib::BindingVisitor &visitor) const override {
        for (const auto &entry : m_bindings) {
            visitor(entry);
        }
    }

private:
    std::optional<modlib::SpriteAsset> spriteByClass(modlib::SpriteID id, modlib::VisualClass expectedClass) const {
        auto asset = sprite(id);
        if (!asset.has_value() || asset->visualClass != expectedClass) {
            return std::nullopt;
        }
        return asset;
    }

    std::optional<modlib::AnimationAsset> animationByClass(modlib::AnimationID id, modlib::VisualClass expectedClass) const {
        auto asset = animation(id);
        if (!asset.has_value() || asset->visualClass != expectedClass) {
            return std::nullopt;
        }
        return asset;
    }

    struct AssetKeyIdx {
        uint64_t kind = 0;
        uint64_t numeric = 0;
        uint64_t symbolic = 0;

        AssetKeyIdx() = default;
        explicit AssetKeyIdx(const modlib::AssetKey &key)
            : kind(key.kind.as_u64), numeric(key.numeric), symbolic(key.symbolic.as_u64) {}

        bool operator==(const AssetKeyIdx &other) const {
            return kind == other.kind && numeric == other.numeric && symbolic == other.symbolic;
        }
    };

    struct BindingKeyIdx {
        uint64_t id = 0;
        AssetKeyIdx key{};

        BindingKeyIdx() = default;
        BindingKeyIdx(modlib::BindingID bindingId, const modlib::AssetKey &assetKey)
            : id(bindingId.as_u64), key(assetKey) {}

        bool operator==(const BindingKeyIdx &other) const {
            return id == other.id && key == other.key;
        }
    };

    struct AssetKeyIdxHash {
        std::size_t operator()(const AssetKeyIdx &key) const {
            std::size_t h = std::hash<uint64_t>{}(key.kind);
            h ^= std::hash<uint64_t>{}(key.numeric) + 0x9e3779b97f4a7c15ULL + (h << 6U) + (h >> 2U);
            h ^= std::hash<uint64_t>{}(key.symbolic) + 0x9e3779b97f4a7c15ULL + (h << 6U) + (h >> 2U);
            return h;
        }
    };

    struct BindingKeyIdxHash {
        std::size_t operator()(const BindingKeyIdx &key) const {
            std::size_t h = std::hash<uint64_t>{}(key.id);
            h ^= AssetKeyIdxHash{}(key.key) + 0x9e3779b97f4a7c15ULL + (h << 6U) + (h >> 2U);
            return h;
        }
    };

    struct StoredSprite {
        modlib::SpriteAsset asset{};
        std::string file{};
        std::string bytesKey{};
    };

    std::unordered_map<uint64_t, StoredSprite> m_sprites;
    std::unordered_map<uint64_t, modlib::AnimationAsset> m_animations;
    std::unordered_map<std::string, std::string> m_fileBytes;
    std::vector<modlib::VisualBinding> m_bindings;
    std::unordered_map<AssetKeyIdx, modlib::VisualBinding, AssetKeyIdxHash> m_bindingByKey;
    std::unordered_map<BindingKeyIdx, modlib::VisualBinding, BindingKeyIdxHash> m_bindingByPair;
    std::unordered_map<uint64_t, std::vector<modlib::VisualBinding>> m_bindingsById;
};

extern "C" Mod *modlib_create(ModManager *) {
    return new AssetManagerModule();
}

