/**
 * \file
 * \brief TCP Connection RAII interface for convinient clients implementation
 * \author Seva Solozhenkin
 * \date 2026-04-29
 */

#pragma once

#include "binmsg.hpp"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <vector>

struct PanFrame
{
	bmsg::Header header{};
	std::vector<uint8_t> payload;

	std::string_view prefix() const;
	std::string_view type() const;
	std::string rawMessage() const;
};

class TcpConnection
{
  private:
	int m_fd = -1;

  public:
	static constexpr int IO_TIMEOUT_MS = 250;

	enum class IoStatus
	{
		OK,
		TIMEOUT,
		CLOSED,
		ERROR,
	};

	TcpConnection() = default;
	TcpConnection(std::string_view host, std::string_view port);
	~TcpConnection();

	TcpConnection(const TcpConnection &) = delete;
	TcpConnection &operator=(const TcpConnection &) = delete;

	TcpConnection(TcpConnection &&other) noexcept;
	TcpConnection &operator=(TcpConnection &&other) noexcept;

	void postInit(std::string_view host, std::string_view port);
	int fd() const;

	IoStatus readFrame(PanFrame &frame, int timeout_ms = IO_TIMEOUT_MS);
	IoStatus writeRaw(std::string_view raw, int timeout_ms = IO_TIMEOUT_MS);
	IoStatus readExact(void *buf, std::size_t len, int timeout_ms = IO_TIMEOUT_MS);
	IoStatus writeExact(const void *buf, std::size_t len, int timeout_ms = IO_TIMEOUT_MS);

  private:
	void closeConnection();
	IoStatus waitForIo(bool want_write, int timeout_ms) const;
};
