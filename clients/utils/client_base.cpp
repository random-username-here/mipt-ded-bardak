#include "client_base.hpp"

#include <ini.h>
#include <cstdarg>
#include <iostream>
#include <string>
#include <string_view>

#include "libpan.h"

#define ESC_GRY "\x1b[90m"
#define ESC_RED "\x1b[91m"
#define ESC_GRN "\x1b[92m"
#define ESC_YLW "\x1b[93m"
#define ESC_BLU "\x1b[94m"
#define ESC_MGN "\x1b[95m"
#define ESC_CYN "\x1b[96m"
#define ESC_RST "\x1b[0m"

struct Refs
{
	PAN &pan;
	std::string &port;
	std::string &host;
};

void l_panLogger(void *data, const char *fmt, ...)
{
	va_list args, copy;
	va_start(args, fmt);
	va_copy(copy, args);
	std::string res;
	res.resize(vsnprintf(nullptr, 0, fmt, copy) + 1);
	vsnprintf(res.data(), res.size(), fmt, args);
	va_end(args);
	std::ostream *ls = (std::ostream *)data;
	if (ls) {
		(*ls) << res;
	}
}

std::ostream &l_logPrefix()
{
	return std::cerr << ESC_GRY << "client ... : " << ESC_RST;
}

int l_iniHandler(void *p, const char *sec_c, const char *name_c, const char *val_c)
{
	Refs *r = (Refs *)p;
	std::string_view sec(sec_c), name(name_c), val(val_c);
	if (sec != "") {
		return 0;
	}
	if (name == "pan_dir") {
		pan_loadDefsFromFile(&r->pan, val_c);
	} else if (name == "host") {
		r->host = val_c;
	} else if (name == "port") {
		r->port = val_c;
	} else {
		l_logPrefix() << ESC_YLW << "Key " << name << " is not known\n" << ESC_RST;
	}
	return 0;
}

ClientBase::ClientBase(const std::string &ini_file_path)
{
	std::string host;
	std::string port;

	Refs r{m_pan, port, host};
	m_pan.userptr = &std::cerr;
	pan_init(&m_pan, l_panLogger, true);
	ini_parse(ini_file_path.c_str(), l_iniHandler, &r);
	m_conn.postInit(host, port);

	registerOnPrefix("role", [this](const PanFrame &frame) { return handleRoleFrame(frame); });
	registerOnPrefix("srv", [this](const PanFrame &frame) { return handleSrvFrame(frame); });
}

bool ClientBase::run()
{
	if (!chooseRole()) {
		std::cerr << roleName() << " role choice error\n";
		return false;
	}
	return readLoop([this] { return true; });
}

bool ClientBase::keepRunning() const
{
	return true;
}

void ClientBase::registerOnPrefix(std::string_view prefix, PrefixCallback callback)
{
	m_prefix_callbacks[prefix] = std::move(callback);
}

void ClientBase::logMessage(PAN_Side side, std::string_view raw)
{
	std::cerr << "\x1b[90mclient " << roleName() << " : \x1b[0m";
	pan_binDump_short(&m_pan, side, raw.data(), raw.size());
	std::cerr << '\n';
}

bool ClientBase::chooseRole()
{
	const std::string_view role = roleName();
	if (!sendMessage(bmsg::CL_role_choose{role})) {
		std::cerr << "send " << role << " role failed\n";
		return false;
	}

	m_role_chosen = false;
	const bool read = readLoop([&] { return !m_role_chosen; });

	return read && m_role_chosen;
}

bool ClientBase::dispatchFrame(const PanFrame &frame)
{
	const auto it = m_prefix_callbacks.find(frame.prefix());
	if (it == m_prefix_callbacks.end()) {
		return true;
	}
	return it->second(frame);
}

bool ClientBase::handleRoleFrame(const PanFrame &frame)
{
	std::cerr << "Recieved role\n";

	const std::string raw = frame.rawMessage();
	bmsg::RawMessage msg(raw);

	if (frame.type() == "chosen") {
		return handleRoleChosenType(msg);
	}
	if (frame.type() == "option") {
		return handleRoleOptionType(msg);
	}
	if (frame.type() == "reject") {
		return handleRoleRejectType(msg);
	}

	return true;
}

bool ClientBase::handleRoleChosenType(bmsg::RawMessage msg)
{
	std::cerr << "Received chosen ";
	const bool decoded = bmsg::SV_role_chosen::decode(msg).has_value();
	std::cerr << "decoded = " << decoded << "\n";
	m_role_chosen = decoded;
	return decoded;
}

bool ClientBase::handleRoleOptionType(bmsg::RawMessage msg)
{
	std::cerr << "Received option ";
	auto decoded = bmsg::SV_role_option::decode(msg);
	if (!decoded.has_value()) {
		std::cerr << "decoded = false\n";
		return false;
	}
	std::cerr << "id = " << decoded->id << " name = " << decoded->name
	          << " prefix = " << decoded->prefix << "\n";
	if (decoded->id == roleName()) {
		std::cerr << "Server has my role: " << roleName() << "\n";
		m_server_has_my_role = true;
	}
	return true;
}

bool ClientBase::handleRoleRejectType(bmsg::RawMessage msg)
{
	const auto reject = bmsg::SV_role_reject::decode(msg);
	std::cerr << "role choice reject, reason: " << (reject ? reject->reason : "none") << '\n';
	return false;
}

bool ClientBase::handleSrvFrame(const PanFrame &frame)
{
	std::cerr << "Received srv\n";

	const std::string raw = frame.rawMessage();
	bmsg::RawMessage msg(raw);
	const std::string_view type = frame.type();

	if (type == "hasPref") {
		return bmsg::SV_srv_hasPref::decode(msg).has_value();
	}
	if (type == "name") {
		return bmsg::SV_srv_name::decode(msg).has_value();
	}
	if (type == "id") {
		return bmsg::SV_srv_id::decode(msg).has_value();
	}
	if (type == "level") {
		return bmsg::SV_srv_level::decode(msg).has_value();
	}
	if (type == "r.setLvl") {
		return bmsg::SV_srv_r_setLvl::decode(msg).has_value();
	}
	return true;
}

template <typename KeepReading>
bool ClientBase::readLoop(KeepReading keep_reading)
{
	while (keep_reading()) {
		PanFrame frame;
		switch (m_conn.readFrame(frame)) {
		case TcpConnection::IoStatus::OK:
			logMessage(PAN_SERVER, frame.rawMessage());
			if (!dispatchFrame(frame)) {
				return false;
			}
			break;
		case TcpConnection::IoStatus::TIMEOUT:
			break;
		case TcpConnection::IoStatus::CLOSED:
			std::cerr << "server closed connection\n";
			return false;
		case TcpConnection::IoStatus::ERROR:
			std::cerr << "tcp read error\n";
			return false;
		}
	}
	return true;
}
