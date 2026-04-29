#pragma once

#include "tcp_connection.h"
#include "../../mods/role_manager/role_proto.hpp"
#include "../../servers/msva/src/srv_proto.hpp"

#include <functional>
#include <iostream>
#include <netdb.h>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

class ClientBase
{
  protected:
	using PrefixCallback = std::function<bool(const PanFrame &)>;

	TcpConnection m_conn;
	bool m_server_has_my_role = false;
	bool m_role_chosen = false;
	std::unordered_map<std::string_view, PrefixCallback> m_prefix_callbacks;

  protected:
	ClientBase(std::string_view host, std::string_view port) : m_conn(host, port)
	{
		if (m_conn.fd() > 0) {
			std::cerr << "Connected to server: " << host << ":" << port << "\n";
		} else {
			std::cerr << "Failed to connect to server: " << host << ":" << port << "\n";
		}

		registerOnPrefix("role", [this](const PanFrame &frame) { return handleRoleFrame(frame); });
		registerOnPrefix("srv", [this](const PanFrame &frame) { return handleSrvFrame(frame); });
	}

  public:
	bool run()
	{
		if (!chooseRole()) {
			std::cerr << roleName() << " role choice error\n";
			return false;
		}
		return readLoop([this] { return true; });
	}

  protected:
	virtual std::string_view roleName() const = 0;

	virtual bool keepRunning() const
	{
		return true;
	}

	void registerOnPrefix(std::string_view prefix, PrefixCallback callback)
	{
		m_prefix_callbacks[prefix] = std::move(callback);
	}

	template <typename Msg>
	bool sendMessage(const Msg &msg)
	{
		std::ostringstream out;
		msg.encode(out, 0, 0);
		return m_conn.writeRaw(out.str()) == TcpConnection::IoStatus::OK;
	}

  private:
	template <typename KeepReading>
	bool readLoop(KeepReading keep_reading)
	{
		while (keep_reading()) {
			PanFrame frame;
			switch (m_conn.readFrame(frame)) {
			case TcpConnection::IoStatus::OK:
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

	virtual bool chooseRole()
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

	virtual bool dispatchFrame(const PanFrame &frame)
	{
		const auto it = m_prefix_callbacks.find(frame.prefix());
		if (it == m_prefix_callbacks.end()) {
			return true;
		}
		return it->second(frame);
	}

	virtual bool handleRoleFrame(const PanFrame &frame)
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

	virtual bool handleRoleChosenType(bmsg::RawMessage msg)
	{
		std::cerr << "Received chosen ";
		const bool decoded = bmsg::SV_role_chosen::decode(msg).has_value();
		std::cerr << "decoded = " << decoded << "\n";
		m_role_chosen = decoded;
		return decoded;
	}

	virtual bool handleRoleOptionType(bmsg::RawMessage msg)
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

	virtual bool handleRoleRejectType(bmsg::RawMessage msg)
	{
		const auto reject = bmsg::SV_role_reject::decode(msg);
		std::cerr << "role choice reject, reason: " << (reject ? reject->reason : "none") << '\n';
		return false;
	}

	virtual bool handleSrvFrame(const PanFrame &frame)
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

	virtual bool handleSrvHasPrefType(bmsg::RawMessage msg)
	{
		std::cerr << "Received hasPref ";
		const bool decoded = bmsg::SV_srv_hasPref::decode(msg).has_value();
		std::cerr << "decoded = " << decoded << "\n";
		m_role_chosen = decoded;
		return decoded;
	}

	virtual bool handleSrvNameType(bmsg::RawMessage msg)
	{
		std::cerr << "Received chosen ";
		const bool decoded = bmsg::SV_role_chosen::decode(msg).has_value();
		std::cerr << "decoded = " << decoded << "\n";
		m_role_chosen = decoded;
		return decoded;
	}
	virtual bool handleSrvIdType(bmsg::RawMessage msg)
	{
	}
	virtual bool handleSrvLevelType(bmsg::RawMessage msg)
	{
	}
	virtual bool handleSrvHasPrefType(bmsg::RawMessage msg)
	{
	}
	virtual bool handleSrvHasPrefType(bmsg::RawMessage msg)
	{
	}
	virtual bool handleSrvHasPrefType(bmsg::RawMessage msg)
	{
	}
};
