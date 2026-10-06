#pragma once
#include "MSVA_client.hpp"
#include "syncv/common.hpp"
#include <functional>

namespace syncv {

using bmsg::Id;

class SyncvClient {
    // TODO: lerp policy: exact (for scores) or lerp (for positions, etc)
public:
    struct VarInfo {
        VarType type;
        union { double prevD; int64_t prevI; };
        union { double curD; int64_t curI; };
        int64_t curTime, prevTime;
        std::string name;

        double val_dbl() const;
        int64_t val_i64() const;
    };
private:
    std::unordered_map<uint64_t, VarInfo> m_vars;

    inline uint64_t m_pack(Id scope, Id index) const {
        return (((uint64_t) scope) << 32) + index;
    }
    inline std::pair<Id, Id> m_unpack(uint64_t v) const {
        return { Id(v >> 32), Id(v) };
    }
public:

    SyncvClient() = default;
    SyncvClient(msva::MsvaClient &cl) { attach(cl); }

    void onMessage(msva::MsvaClient &client, bmsg::RawMessage msg);
    
    void attach(msva::MsvaClient &client) {
        client.registerHandler("syncv", [this, &client](bmsg::RawMessage msg){
            onMessage(client, msg);
        });
    }

    void forEachVar(const std::function<void(Id, Id, const VarInfo&)> &f) const {
        for (const auto &[id, vi] : m_vars) {
            auto [sc, idx] = m_unpack(id);
            f(sc, idx, vi);
        }
    }

    double getDouble(Id sc, Id idx, double def = 0) const;
    int64_t getInt(Id sc, Id idx, int64_t def = 0) const;
    float getFloat(Id sc, Id idx, float def = 0) const { return getDouble(sc, idx, def); }

    // clear after server disconnect
    void forceClear() {
        m_vars.clear();
    }
};

};
