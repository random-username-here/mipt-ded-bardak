#include "AssetManager.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

static std::string loadFileBytes(const std::filesystem::path &p) {
    std::ifstream ifs(p, std::ios::binary);
    if (!ifs) return {};

    ifs.seekg(0, std::ios::end);
    std::streamsize n = ifs.tellg();
    if (n <= 0) return {};
    ifs.seekg(0, std::ios::beg);

    std::string data;
    data.resize((size_t)n);
    if (!ifs.read(data.data(), n)) return {};
    return data;
}

static uint64_t rotKey(modlib::AssetId baseId, modlib::AssetTransform tr) {
    // Stable key: baseId + exact rotation milli-degrees.
    return (uint64_t(baseId) << 32) ^ uint32_t(tr.rotationMilliDeg);
}

} // namespace

class AssetManagerModule final : public modlib::AssetManager {
public:
    std::string_view id() const override { return "isd.bardak.asset_manager"; }
    std::string_view brief() const override { return "Registers textures and serves bytes by assetId"; }
    modlib::ModVersion version() const override { return modlib::ModVersion(0, 1, 0); }

    // Register base texture by file path, return a new asset id.
    // If same file path is registered multiple times, returns the first id.
    modlib::AssetId addTexture(modlib::AssetKind kind, std::string_view filePath) override {
        std::string pathStr(filePath);
        // Normalize for cache key stability.
        std::filesystem::path p(pathStr);
        p = p.lexically_normal();
        pathStr = p.string();

        auto it = m_pathToBase.find(pathStr);
        if (it != m_pathToBase.end()) {
            return it->second;
        }

        std::string bytes = loadFileBytes(p);
        if (bytes.empty()) return 0;

        modlib::AssetId id = (modlib::AssetId)m_assets.size();
        m_assets.push_back(AssetRecord{
            kind,
            id,
            modlib::AssetTransform{},
            std::move(bytes),
            pathStr,
        });
        m_pathToBase[pathStr] = id;
        return id;
    }

    std::string_view getTextureBytes(modlib::AssetId id) const override {
        if (!valid(id)) return {};
        const AssetRecord &rec = m_assets[id];
        if (!valid(rec.baseId)) return {};
        return m_assets[rec.baseId].bytes;
    }

    modlib::AssetTransform getTransform(modlib::AssetId id) const override {
        if (!valid(id)) return {};
        return m_assets[id].transform;
    }

    modlib::AssetId getTransformed(modlib::AssetId baseId, modlib::AssetTransform transform) override {
        if (!valid(baseId)) return 0;
        // Cache transformed "views" so we don't create up to 360 variants unless needed.
        // We still avoid duplicating bytes: transformed ids reference the same baseId.
        uint64_t key = rotKey(baseId, transform);
        auto it = m_transformedCache.find(key);
        if (it != m_transformedCache.end()) return it->second;

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

    bool valid(modlib::AssetId id) const {
        return id < (modlib::AssetId)m_assets.size();
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

