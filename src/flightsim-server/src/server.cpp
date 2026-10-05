#include "MSVA.hpp"
#include "modlib_manager.hpp"
#include <cmath>
#include "syncv/server.hpp"

class FlightsimServer : public msva::MsvaModule {
public:

    std::string_view id() const override { return "isd.flightsim"; };
    std::string_view brief() const override { return "A flight simulator game"; };
    ModVersion version() const override { return ModVersion(0, 0, 1); }

    syncv::SyncvModule *m_sv;
    int64_t cnt = 0;

    void onResolveDeps(ModManager *mm) override {
        m_sv = mm->anyOfType<syncv::SyncvModule>();
    }

    void onDepsResolved(ModManager *) override {
        m_sv->createVar(0, 0, syncv::VarType::DOUBLE, true, "x");
        m_sv->createVar(0, 1, syncv::VarType::DOUBLE, true, "y");
        m_sv->createVar(0, 2, syncv::VarType::INT64, true, "counter");
    }

    void onIteration() override {

        const double rotTime = 1e9; // 1s

        auto now = msva::nsTime();
        m_sv->setVar(0, 0, std::cos(now / rotTime));
        m_sv->setVar(0, 1, std::sin(now / rotTime));
        m_sv->setVar(0, 2, cnt++);
    }
};

extern "C" Mod* modlib_create() {
    return new FlightsimServer();
}

