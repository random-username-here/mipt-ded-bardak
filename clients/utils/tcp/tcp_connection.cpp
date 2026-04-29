#include "utils/tcp/tcp_connection.hpp"

#include <cstring>
#include <stdexcept>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>

namespace
{
int connect(std::string_view host, std::string_view port)
{
	addrinfo hints{};
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_UNSPEC;

	addrinfo *res = nullptr;
	const std::string host_string(host);
	const std::string port_string(port);
	const int addr_info = ::getaddrinfo(host_string.c_str(), port_string.c_str(), &hints, &res);
	if (addr_info != 0) {
		throw std::runtime_error("getaddrinfo(" + host_string + ", " + port_string +
		                         "): " + ::gai_strerror(addr_info));
	}

	int fd = -1;
	for (addrinfo *it = res; it != NULL; it = it->ai_next) {
		fd = ::socket(it->ai_family, it->ai_socktype, it->ai_protocol);
		if (fd < 0) {
			continue;
		}

		if (::connect(fd, it->ai_addr, it->ai_addrlen) == 0) {
			break;
		}

		close(fd);
		fd = -1;
	}

	freeaddrinfo(res);

	if (fd < 0) {
		throw std::runtime_error("connect(" + host_string + ", " + port_string + ") failed");
	}

	return fd;
}

template <typename WaitFunc, typename Callback>
TcpConnection::IoStatus doExact(WaitFunc wf, Callback cb, std::size_t len)
{
	std::size_t offset = 0;
	while (offset < len) {
		const TcpConnection::IoStatus wait_status = wf();
		if (wait_status != TcpConnection::IoStatus::OK) {
			return wait_status;
		}

		const ssize_t count = cb(offset);
		if (count == 0) {
			return TcpConnection::IoStatus::CLOSED;
		}
		if (count < 0) {
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
				continue;
			}
			return TcpConnection::IoStatus::ERROR;
		}
		offset += static_cast<std::size_t>(count);
	}
	return TcpConnection::IoStatus::OK;
}

} // namespace

std::string_view PanFrame::prefix() const
{
	return std::string_view(header.pref.as_chars, header.pref.size());
}

std::string_view PanFrame::type() const
{
	return std::string_view(header.type.as_chars, header.type.size());
}

std::string PanFrame::rawMessage() const
{
	bmsg::Header raw_header = header;
	raw_header.len = static_cast<uint16_t>(payload.size());

	std::string raw(sizeof(bmsg::Header) + payload.size(), '\0');
	std::memcpy(raw.data(), &raw_header, sizeof(raw_header));
	if (!payload.empty()) {
		std::memcpy(raw.data() + sizeof(bmsg::Header), payload.data(), payload.size());
	}
	return raw;
}

TcpConnection::TcpConnection(std::string_view host, std::string_view port)
    : m_fd(connect(host, port))
{
}

TcpConnection::~TcpConnection()
{
	closeConnection();
}

TcpConnection::TcpConnection(TcpConnection &&other) noexcept : m_fd(other.m_fd)
{
	other.m_fd = -1;
}

TcpConnection &TcpConnection::operator=(TcpConnection &&other) noexcept
{
	if (this != &other) {
		closeConnection();
		m_fd = other.m_fd;
		other.m_fd = -1;
	}
	return *this;
}

void TcpConnection::postInit(std::string_view host, std::string_view port)
{
	m_fd = connect(host, port);
}

int TcpConnection::fd() const
{
	return m_fd;
}

TcpConnection::IoStatus TcpConnection::readFrame(PanFrame &frame, int timeout_ms)
{
	const IoStatus header_status = readExact(&frame.header, sizeof(frame.header), timeout_ms);
	if (header_status != IoStatus::OK) {
		return header_status;
	}

	frame.payload.clear();
	frame.payload.resize(frame.header.len);
	return readExact(frame.payload.data(), frame.payload.size(), timeout_ms);
}

TcpConnection::IoStatus TcpConnection::writeRaw(std::string_view raw, int timeout_ms)
{
	return writeExact(raw.data(), raw.size(), timeout_ms);
}

TcpConnection::IoStatus TcpConnection::readExact(void *buf, std::size_t len, int timeout_ms)
{
	return doExact([this, timeout_ms]() { return waitForIo(false, timeout_ms); },
	               [this, buf, len](std::size_t offset) {
		               return ::recv(m_fd, static_cast<uint8_t *>(buf) + offset, len - offset, 0);
	               },
	               len);
}

TcpConnection::IoStatus TcpConnection::writeExact(const void *buf, std::size_t len, int timeout_ms)
{
	return doExact([this, timeout_ms]() { return waitForIo(true, timeout_ms); },
	               [this, buf, len](std::size_t offset) {
		               return ::send(m_fd, static_cast<const uint8_t *>(buf) + offset, len - offset,
		                             MSG_NOSIGNAL);
	               },
	               len);
}

void TcpConnection::closeConnection()
{
	if (m_fd >= 0) {
		::close(m_fd);
		m_fd = -1;
	}
}

TcpConnection::IoStatus TcpConnection::waitForIo(bool want_write, int timeout_ms) const
{
	while (true) {
		fd_set rfds;
		fd_set wfds;
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);

		if (want_write) {
			FD_SET(m_fd, &wfds);
		} else {
			FD_SET(m_fd, &rfds);
		}

		timeval tv{};
		tv.tv_sec = timeout_ms / 1000;
		tv.tv_usec = (timeout_ms % 1000) * 1000;

		const int rc = ::select(m_fd + 1, want_write ? nullptr : &rfds,
		                        want_write ? &wfds : nullptr, nullptr, &tv);
		if (rc > 0) {
			return IoStatus::OK;
		}
		if (rc == 0) {
			return IoStatus::TIMEOUT;
		}
		if (errno != EINTR) {
			return IoStatus::ERROR;
		}
	}
}
