#pragma once
#include "MSVA.hpp"
#include "syncv/common.hpp"
#include <unordered_set>

namespace syncv {

using bmsg::Id;

class SyncvModule : public msva::MsvaModule {

    struct VarInfo {
        VarType type;
        union { double d; int64_t i; };
        std::string name;
        bool useTcp;
        int64_t updateTime;
    };

    std::unordered_map<uint64_t, VarInfo> m_vars;
    std::unordered_set<uint64_t> m_pending;
    int64_t m_updateInterval = 25000000; // 25ms
    int64_t m_lastUpdated = 0;

    inline uint64_t m_pack(Id scope, Id index) const {
        return (((uint64_t) scope) << 32) + index;
    }
    inline std::pair<Id, Id> m_unpack(uint64_t v) const {
        return { Id(v >> 32), Id(v) };
    }

public:

    std::string_view id() const override { return "msva.syncv"; };
    std::string_view brief() const override { return "Sync variables over network"; };
    ModVersion version() const override { return ModVersion(0, 0, 1); }

protected:
    void onSetup(msva::MsvaServer *) override;
    void onConnect(msva::MsvaUser *) override;
    void onIteration() override;

public:

    void setUpdateInterval(int64_t ui) { m_updateInterval = ui; }

    void createVar(
        bmsg::Id scope, bmsg::Id index, VarType type,
        bool useUdp = true, std::string_view name = ""
    );
    void removeVar(bmsg::Id scope, bmsg::Id index);
    void setVar(bmsg::Id scope, bmsg::Id index, int64_t val);
    void setVar(bmsg::Id scope, bmsg::Id index, double val);

};

};
