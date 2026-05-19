/**
 * Public msva api, for extra features not from standard.
 */
#pragma once

#include "BmServerModule.hpp"
#include <functional>
#include <netinet/in.h>
#include <optional>
#include <string>
#include <string_view>
#include <sstream>

namespace msva {

using LogFunc = std::function<void(std::string_view)>;

class LogStream {
    std::ostringstream m_os;
    LogFunc m_lf;

public:

    LogStream(LogFunc lf) :m_lf(lf) {}

    LogStream(const LogStream &l) = delete;
    LogStream &operator=(const LogStream &l) = delete;
    LogStream(LogStream &&l) { m_lf = l.m_lf; m_os = std::move(l.m_os); l.m_lf = nullptr; }
    LogStream &operator=(LogStream &&l) { m_lf = l.m_lf; m_os = std::move(l.m_os); l.m_lf = nullptr; return *this; }

    ~LogStream() {
        if (m_lf) m_lf(m_os.str());
    }

    std::ostream &stream() { return m_os; }

    template<typename T>
    std::ostream &operator<<(const T& v) {
        return m_os << v;
    }
};


class Server : public modlib::BmServer {
public:
    virtual void setTTYLogs(bool enabled) = 0;
    virtual void setLogCallback(LogFunc fn) = 0;
    virtual LogFunc logCallback() = 0;
    virtual void setConfigValue(std::string_view key, std::string_view value) = 0;
    virtual std::optional<std::string> configValue(std::string_view key) const = 0;
    virtual void disconnectClient(modlib::BmClient *client) = 0;
    virtual void disconnectClient(size_t clientId) = 0;
};

class Client : public modlib::BmClient {
public:
    virtual sockaddr_in addr() const = 0;
    virtual ~Client() = default;
};

};
