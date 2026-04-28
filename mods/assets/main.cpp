#include "BmServerModule.hpp"
#include "binmsg.hpp"
#include "modlib_manager.hpp"

#include "./asset_proto.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using namespace modlib;

namespace {

enum class AssetKind : uint8_t { Tile = 0, Unit = 1 };

static std::string readFileBytes(const std::filesystem::path &p) {
    std::ifstream ifs(p, std::ios::binary);
    if (!ifs) return {};
    std::string data;
    ifs.seekg(0, std::ios::end);
    std::streamsize n = ifs.tellg();
    if (n <= 0) return {};
    ifs.seekg(0, std::ios::beg);
    data.resize((size_t)n);
    if (!ifs.read(data.data(), n)) return {};
    return data;
}

static std::string encodeRaw(std::string_view pref, std::string_view type, bmsg::Id id, uint16_t flags, std::string_view body) {
    bmsg::Header hdr = {};
    hdr.pref = pref;
    hdr.type = type;
    hdr.id = id;
    hdr.len = (uint16_t)body.size();
    hdr.flags = flags;

    std::string out;
    out.resize(sizeof(hdr) + body.size());
    std::memcpy(out.data(), &hdr, sizeof(hdr));
    if (!body.empty())
        std::memcpy(out.data() + sizeof(hdr), body.data(), body.size());
    return out;
}

static std::string encodeAssetData(AssetKind kind, uint32_t typeId, std::string_view bytes) {
    // Body format:
    //  u8  kind (0 tile, 1 unit)
    //  u32 typeId
    //  u32 size
    //  u8[size] bytes
    std::string body;
    body.resize(sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) + bytes.size());
    size_t pos = 0;
    body[pos] = (char)kind;
    pos += sizeof(uint8_t);
    std::memcpy(body.data() + pos, &typeId, sizeof(typeId));
    pos += sizeof(typeId);
    uint32_t sz = (uint32_t)bytes.size();
    std::memcpy(body.data() + pos, &sz, sizeof(sz));
    pos += sizeof(sz);
    if (!bytes.empty())
        std::memcpy(body.data() + pos, bytes.data(), bytes.size());
    return encodeRaw("ast", "data", 0, 0, body);
}

} // namespace

class AssetManager : public BmServerModule {
    std::filesystem::path m_root;
    std::unordered_map<uint32_t, std::string> m_tileCache;
    std::unordered_map<uint32_t, std::string> m_unitCache;

public:
    AssetManager() : m_root(std::filesystem::path("assets")) {}

    std::string_view id() const override { return "isd.bardak.asset_manager"; }
    std::string_view brief() const override { return "Serves tile/unit textures to clients"; }
    ModVersion version() const override { return ModVersion(0, 1, 0); }

    void onSetup(BmServer *server) override {
        server->registerPrefix("ast", this);
    }

    void onMessage(BmClient *cl, bmsg::RawMessage msg) override {
        assert(msg.isCorrect());
        if (msg.header()->pref != "ast") return;

        const auto type = (std::string_view)msg.header()->type;
        if (type == "tile") {
            auto req = bmsg::CL_ast_tile::decode(msg);
            if (!req) return;
            handleGet(cl, AssetKind::Tile, req->typeId);
        } else if (type == "unit") {
            auto req = bmsg::CL_ast_unit::decode(msg);
            if (!req) return;
            handleGet(cl, AssetKind::Unit, req->typeId);
        }
    }

private:
    void handleGet(BmClient *cl, AssetKind kind, uint32_t typeId) {
        std::string *cached = nullptr;
        if (kind == AssetKind::Tile) {
            cached = &m_tileCache[typeId];
            if (cached->empty()) {
                *cached = readFileBytes(m_root / "tiles" / (std::to_string(typeId) + ".png"));
            }
        } else {
            cached = &m_unitCache[typeId];
            if (cached->empty()) {
                *cached = readFileBytes(m_root / "units" / (std::to_string(typeId) + ".png"));
            }
        }

        if (cached->empty()) {
            cl->send(bmsg::SV_ast_missing { (uint8_t)kind, typeId });
            return;
        }

        const std::string raw = encodeAssetData(kind, typeId, *cached);
        cl->send(bmsg::RawMessage(raw));
    }
};

extern "C" Mod *modlib_create(ModManager *) {
    return new AssetManager();
}

