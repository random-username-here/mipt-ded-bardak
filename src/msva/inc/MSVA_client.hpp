/**
 * \file
 * \brief Client for modular server
 * \author Didyk Ivan
 * \date 2026-10-02
 */
#pragma once
#include "binmsg.hpp"
#include "libpan.h"
#include <cstdint>
#include <filesystem>
#include <netinet/in.h>
#include <unordered_map>
#include <functional>
#include <string>
#include <sstream>

namespace msva {

int64_t nsTime();

enum class MsvaClientState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED
};

class MsvaClient {
    std::unordered_map<uint64_t, std::function<void(bmsg::RawMessage)>> m_prefMapping;

    std::string m_host;
    size_t m_port;
    sockaddr_in m_serverAddr;
    
    PAN m_pan;
    bool m_loggingEnabled = true;

    std::string m_tcp_inBuffer;
    int m_tcp;
    int m_udp;
    size_t m_seq = 1;
    std::string m_buf;
    MsvaClientState m_state = MsvaClientState::DISCONNECTED;

    std::string m_srvName;
    bmsg::Id m_id = -1;
    int64_t m_timeDelta = 0;
    int64_t m_delay = 0;
    int64_t m_lastTimeSynced = 0;
    int64_t m_syncInterval = 100000000; // 0.1s
    size_t m_maxMsgsPerFrame = 128;

    size_t m_processMessage(std::string_view msg);
    void m_serverMessage(bmsg::RawMessage rm);

public:

    MsvaClient();

    void enableLogs(bool en) { m_loggingEnabled = en; }
    void registerHandler(bmsg::Char64 prefix, std::function<void(bmsg::RawMessage)> cb);
    void loadProtoDefs(std::filesystem::path path);
    void setTimeSyncInterval(int64_t tsi) { m_syncInterval = tsi; }
    void setMaxMsgsPerFrame(size_t n) { m_maxMsgsPerFrame = 128; }

    MsvaClientState state() const { return m_state; }

    bool connect(std::string_view addr);
    void disconnect();
    bool poll(); // returns false if connection is broken
    
    void send(bmsg::RawMessage msg);
    template<typename T>
    void send(const T& msg, uint16_t flags = 0) {
        std::ostringstream oss;
        msg.encode(oss, m_seq++, flags);
        send(bmsg::RawMessage(oss.str()));
    }

    uint64_t time() const { return nsTime() + m_timeDelta; }

    /// Time, which should be added to local to approximate server time
    int64_t delta() const { return m_timeDelta; }

    int64_t delay() const { return m_delay; }

    bmsg::Id id() const { return m_id; }
    std::string_view serverName() const { return m_srvName; }

    bool hasId() const { return m_id != -1; }

    MsvaClient(const MsvaClient &o) = delete;
    MsvaClient& operator=(const MsvaClient &o) = delete;
    MsvaClient(MsvaClient &&o) = default;
    MsvaClient& operator=(MsvaClient &&o) = default;
    ~MsvaClient();
};

};
