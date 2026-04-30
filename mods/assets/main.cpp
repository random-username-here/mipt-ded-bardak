#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "modlib_mod.hpp"
#include "AssetManager.hpp"

namespace {

static std::string loadFileBytes(const std::filesystem::path &filePath) {
    std::ifstream inputFile(filePath, std::ios::binary);
    if (!inputFile) return {};

    inputFile.seekg(0, std::ios::end);
    std::streamsize fileSize = inputFile.tellg();
    if (fileSize <= 0) return {};
    inputFile.seekg(0, std::ios::beg);

    std::string dataBytes;
    dataBytes.resize((size_t)fileSize);
    if (!inputFile.read(dataBytes.data(), fileSize)) return {};
    return dataBytes;
}

static uint64_t transformedKey(modlib::AssetId baseId, modlib::AssetTransform transform) {
    // Stable key: baseId + exact rotation milli-degrees.
    return (uint64_t(baseId) << 32) ^ uint32_t(transform.rotationMilliDeg);
}

} // namespace

class AssetManagerModule final : public modlib::AssetManager {
public:
    AssetManagerModule() {
        // Reserve id=0 as "invalid asset id".
        m_assets.emplace_back();
    }

    std::string_view id() const override { return "neilor.bardak.asset_manager"; }
    std::string_view brief() const override { return "Registers textures and serves bytes by assetId"; }
    ModVersion version() const override { return ModVersion(0, 1, 0); }

    // Register base texture by file path, return a new asset id.
    // If same file path is registered multiple times, returns the first id.
    modlib::AssetId addTexture(modlib::AssetKind kind, std::string_view filePath) override {
        std::string normalizedPath(filePath);
        // Normalize for cache key stability.
        std::filesystem::path fileSystemPath(normalizedPath);
        fileSystemPath = fileSystemPath.lexically_normal();
        normalizedPath = fileSystemPath.string();

        auto cachedEntry = m_pathToBase.find(normalizedPath);
        if (cachedEntry != m_pathToBase.end()) {
            return cachedEntry->second;
        }

        std::string textureBytes = loadFileBytes(fileSystemPath);
        if (textureBytes.empty()) return modlib::kInvalidAssetId;

        modlib::AssetId id = (modlib::AssetId)m_assets.size();
        m_assets.push_back(AssetRecord{
            kind,
            id,
            modlib::AssetTransform{},
            std::move(textureBytes),
            normalizedPath,
        });
        m_pathToBase[normalizedPath] = id;
        return id;
    }

    std::string_view getTextureBytes(modlib::AssetId id) const override {
        if (!isValidAssetId(id)) return {};
        const AssetRecord &rec = m_assets[id];
        if (!isValidAssetId(rec.baseId)) return {};
        return m_assets[rec.baseId].bytes;
    }

    modlib::AssetTransform getTransform(modlib::AssetId id) const override {
        if (!isValidAssetId(id)) return {};
        return m_assets[id].transform;
    }

    modlib::AssetId getTransformed(modlib::AssetId baseId, modlib::AssetTransform transform) override {
        if (!isValidAssetId(baseId)) return modlib::kInvalidAssetId;
        // Cache transformed "views" so we don't create up to 360 variants unless needed.
        // We still avoid duplicating bytes: transformed ids reference the same baseId.
        uint64_t key = transformedKey(baseId, transform);
        auto cachedEntry = m_transformedCache.find(key);
        if (cachedEntry != m_transformedCache.end()) return cachedEntry->second;

        modlib::AssetId id = (modlib::AssetId)m_assets.size();
        m_assets.push_back(AssetRecord{
            m_assets[baseId].kind,
            baseId,
            transform,
            std::string{}, // no duplication of bytes
            std::string{},
        });
        m_transformedCache.emplace(key, id);
        return id;
    }

private:
    struct AssetRecord {
        modlib::AssetKind kind = modlib::AssetKind::Tile;
        modlib::AssetId baseId = 0;           // points to record which holds bytes
        modlib::AssetTransform transform{};  // metadata for visualization
        std::string bytes;                   // only filled for base textures
        std::string path;                    // only filled for base textures (debug)
    };

    bool isValidAssetId(modlib::AssetId id) const {
        return id != modlib::kInvalidAssetId && id < (modlib::AssetId)m_assets.size();
    }

    std::vector<AssetRecord> m_assets;
    // Map "normalized path" -> base assetId
    std::unordered_map<std::string, modlib::AssetId> m_pathToBase;
    // Cache transformed views: (baseId, rotationMilliDeg) -> transformed assetId
    std::unordered_map<uint64_t, modlib::AssetId> m_transformedCache;
};

extern "C" Mod *modlib_create(ModManager *) {
    return new AssetManagerModule();
}

