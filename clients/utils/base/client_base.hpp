/**
 * \file
 * \brief Client base interface for convinient clients implementation
 * \author Seva Solozhenkin
 * \date 2026-04-29
 */
#pragma once

#include "libpan.h"
#include "utils/tcp/tcp_connection.hpp"

#include <functional>
#include <iostream>
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
	PAN m_pan{};
	std::unordered_map<std::string_view, PrefixCallback> m_prefix_callbacks;

  protected:
	ClientBase(const std::string &ini_file_path);

  public:
	bool run();

  protected:
	virtual std::string_view roleName() const = 0;

	virtual bool keepRunning() const;

	void registerOnPrefix(std::string_view prefix, PrefixCallback callback);

	template <typename Msg>
	bool sendMessage(const Msg &msg);

  private:
	void logMessage(PAN_Side side, std::string_view raw);

	template <typename KeepReading>
	bool readLoop(KeepReading keep_reading);

	virtual bool chooseRole();
	virtual bool dispatchFrame(const PanFrame &frame);
	virtual bool handleRoleFrame(const PanFrame &frame);
	virtual bool handleRoleChosenType(bmsg::RawMessage msg);
	virtual bool handleRoleOptionType(bmsg::RawMessage msg);
	virtual bool handleRoleRejectType(bmsg::RawMessage msg);
	virtual bool handleSrvFrame(const PanFrame &frame);
};

template <typename Msg>
bool ClientBase::sendMessage(const Msg &msg)
{
	std::ostringstream out;
	msg.encode(out, 0, 0);
	const std::string raw = out.str();
	logMessage(PAN_CLIENT, raw);
	return m_conn.writeRaw(out.str()) == TcpConnection::IoStatus::OK;
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
