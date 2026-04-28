#pragma once

#include "tcp_connection.h"

#include <stdexcept>
#include <string>
#include <unistd.h>

class TcpConnection {
public:
	TcpConnection(const char *host, const char *port)
		: m_fd(connect_tcp(host, port))
	{
		if (m_fd < 0) {
			throw std::runtime_error("connect_tcp failed");
		}
	}

	~TcpConnection()
	{
		if (m_fd >= 0) {
			close(m_fd);
		}
	}

	TcpConnection(const TcpConnection&) = delete;
	TcpConnection& operator=(const TcpConnection&) = delete;

	TcpConnection(TcpConnection&& other) noexcept
		: m_fd(other.m_fd)
	{
		other.m_fd = -1;
	}

	TcpConnection& operator=(TcpConnection&& other) noexcept
	{
		if (this == &other) {
			return *this;
		}
		if (m_fd >= 0) {
			close(m_fd);
		}
		m_fd = other.m_fd;
		other.m_fd = -1;
		return *this;
	}

	int fd() const { return m_fd; }

	int readHeader(PanHeader *hdr, int timeout_ms = IO_TIMEOUT_MS)
	{
		return read_pan_header(m_fd, hdr, timeout_ms);
	}

	int readExact(void *buf, size_t len, int timeout_ms = IO_TIMEOUT_MS)
	{
		return read_exact(m_fd, buf, len, timeout_ms);
	}

	int writeExact(const void *buf, size_t len, int timeout_ms = IO_TIMEOUT_MS)
	{
		return write_exact(m_fd, buf, len, timeout_ms);
	}

	int writeRaw(const std::string& raw, int timeout_ms = IO_TIMEOUT_MS)
	{
		return writeExact(raw.data(), raw.size(), timeout_ms);
	}

private:
	int m_fd = -1;
};
