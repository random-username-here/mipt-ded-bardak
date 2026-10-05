#include "syncv/client.hpp"
#include "syncv/proto.hpp"
#include "aixlog.hpp"

namespace syncv {

// TODO: smooth them a bit with more points

double SyncvClient::VarInfo::val_dbl() const {
    assert(type == VarType::DOUBLE);
    if (curTime == 0) return 0;
    //return curD;
    if (prevTime == 0) return curD;
    return curD + (curD - prevD) * (msva::nsTime() - curTime) / (curTime - prevTime);
}

int64_t SyncvClient::VarInfo::val_i64() const {
    assert(type == VarType::INT64);
    if (curTime == 0) return 0;
    if (prevTime == 0) return curI;
    return curI + (curI - prevI) * (msva::nsTime() - curTime) / (curTime - prevTime);
}

void SyncvClient::onMessage(msva::MsvaClient &client, bmsg::RawMessage msg) {
    if (msg.header()->pref != "syncv") return;
    if (msg.header()->type == "declare") {
        auto m = bmsg::SV_syncv_declare::decode(msg);
        if (!m) return;
        auto id = m_pack(m->scope, m->var);
        LOG(DEBUG, "syncv") << "Declared syncv " << (char) m->type << " " << m->scope << ":" << m->var 
            << " `" << m->name << "`\n";

        m_vars[id] = VarInfo {
            .type = VarType(m->type),
            .prevI = 0, .curI = 0,
            .curTime = 0, .prevTime = 0,
            .name = std::string(m->name),
        };
    } else if (msg.header()->type == "remove") {
        auto m = bmsg::SV_syncv_remove::decode(msg);
        if (!m) return;
        LOG(DEBUG, "syncv") << "Removed syncv " << m->scope << ":" << m->var << '\n';
        auto id = m_pack(m->scope, m->var);
        m_vars.erase(id);
    } else if (msg.header()->type == "update") {
        auto m = bmsg::SV_syncv_update::decode(msg);
        if (!m) return;
        auto id = m_pack(m->scope, m->var);
        if (!m_vars.count(id)) return;
        auto &v = m_vars.at(id);
        int64_t corrected = m->time - client.delta();
        if (corrected > v.curTime) {
            v.prevI = v.curI;
            v.prevTime = v.curTime;
            v.curI = m->val;
            v.curTime = corrected;
        } else if (corrected > v.prevTime) {
            v.prevI = m->val;
            v.prevTime = corrected;
        }
    }
}

double SyncvClient::getDouble(Id sc, Id idx, double def) const {
    auto id = m_pack(sc, idx);
    if (!m_vars.count(id)) return def;
    auto &v = m_vars.at(id);
    if (v.curTime == 0) return def;
    return v.val_dbl();
}

int64_t SyncvClient::getInt(Id sc, Id idx, int64_t def) const {
    auto id = m_pack(sc, idx);
    if (!m_vars.count(id)) return def;
    auto &v = m_vars.at(id);
    if (v.curTime == 0) return def;
    return v.val_i64();
}

};
