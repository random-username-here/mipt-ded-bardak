#include "syncv/server.hpp"
#include "syncv/proto.hpp"
#include "aixlog.hpp"

namespace syncv {

void SyncvModule::onSetup(msva::MsvaServer *srv) {
    srv->registerPrefix("syncv", this);
}

void SyncvModule::onConnect(msva::MsvaUser *user) {
    for (const auto &[id, var] : m_vars) {
        auto [scope, idx] = m_unpack(id);
        user->send(bmsg::SV_syncv_declare { scope, idx, (int8_t) var.type, var.name });
    }
}

void SyncvModule::onIteration() {
    int64_t t = msva::nsTime();
    if (m_lastUpdated + m_updateInterval >= t) return;
    m_lastUpdated = t;
    LOG(DEBUG, "syncv") << "Sending " << m_pending.size() << " vars\n";
    server()->forAllClients([this](msva::MsvaUser *user){
        for (auto id : m_pending) {
            auto [scope, idx] = m_unpack(id);
            const auto &var = m_vars[id];
            user->send(bmsg::SV_syncv_update {
                scope, idx, var.updateTime, var.i
            }, (var.useTcp ? bmsg::Flags(0) : bmsg::USE_UDP));
        }
    });
    m_pending.clear();
}

void SyncvModule::createVar(
    bmsg::Id scope, bmsg::Id idx, VarType type,
    bool useUdp, std::string_view name
) {
    LOG(DEBUG, "syncv") << "Created syncv " << (char) type << " " << scope << ":" << idx 
        << " `" << name << "`" << (useUdp ? " via udp" : " via tcp") << '\n';
    uint64_t id = m_pack(scope, idx);
    assert(!m_vars.count(id)); // no double declaring
    m_vars[id] = VarInfo {
        .type = type,
        .i = 0,
        .name = std::string(name),
        .useTcp = !useUdp,
        .updateTime = msva::nsTime()
    };
    if (server()) {
        server()->forAllClients([this, scope, idx, type, name](msva::MsvaUser *user){
            user->send(bmsg::SV_syncv_declare { scope, idx, (int8_t) type, name });
        });
    }
}

void SyncvModule::removeVar(bmsg::Id scope, bmsg::Id idx) {
    LOG(DEBUG, "syncv") << "Removed syncv " << scope << ":" << idx << '\n';

    uint64_t id = m_pack(scope, idx);
    assert(m_vars.count(id));
    m_vars.erase(id);
    if (server()) {
        server()->forAllClients([this, scope, idx](msva::MsvaUser *user){
            user->send(bmsg::SV_syncv_remove { scope, idx });
        });
    }
}

void SyncvModule::setVar(bmsg::Id scope, bmsg::Id idx, int64_t val) {
    uint64_t id = m_pack(scope, idx);
    assert(m_vars.count(id));
    auto &v = m_vars.at(id);
    if (v.i == val) return;
    m_vars[id].i = val;
    m_vars[id].updateTime = msva::nsTime();
    m_pending.insert(id);
}

void SyncvModule::setVar(bmsg::Id scope, bmsg::Id index, double val) {
    union { double d; int64_t i; }; 
    // FIXME: temporary hack, make better way for sending doubles
    d = val;
    setVar(scope, index, i);
}

extern "C" Mod* modlib_create() {
    return new SyncvModule();
}

};
