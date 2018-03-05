#include <arpa/inet.h>
#include "socket.hpp"
#include <stdexcept>
#include <unistd.h>
#include <cstring>
#include <iostream>

TcpSocket::TcpSocket(const std::string &dest, uint16_t port)
		: m_addr { dest }, m_port { port } {
	if (!openSocket()) {
		throw std::runtime_error { "Failed to open socket" };
	}
}

TcpSocket::TcpSocket()
		: m_fd { -1 }, m_addr { "" }, m_port { 0 } {
}

TcpSocket::~TcpSocket() {
	close();
}

void TcpSocket::close() {
	if (m_fd != -1) {
		shutdown(m_fd, SHUT_RDWR);
		::close(m_fd);
		m_fd = -1;
	}
}

bool TcpSocket::openSocket() {
	m_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (m_fd == -1) {
		return false;
	}

	memset(&m_sockaddr, 0, sizeof(struct sockaddr_in));

	m_sockaddr.sin_port = htons(m_port);
	inet_aton(m_addr.c_str(), &m_sockaddr.sin_addr);
	m_sockaddr.sin_family = AF_INET;

	if (connect(m_fd, reinterpret_cast<struct sockaddr *>(&m_sockaddr),
			sizeof(struct sockaddr_in)) != 0) {
		::close(m_fd);
		m_fd = -1;
		return false;
	}

	return true;
}

void TcpSocket::fromDescriptor(const sockaddr_in &sa, int fd) {
	m_port = ntohs(sa.sin_port);
	char buf[64];
	const char *res = nullptr;
	if ((res = inet_ntop(AF_INET, &sa.sin_addr, buf, 64))) {
		m_addr = std::string(res);
	}
	// else error

	m_fd = fd;
	m_sockaddr = sa;
}

ssize_t TcpSocket::read(void *buf, size_t sz) {
	if (m_fd == -1) {
		throw std::runtime_error { "Read from closed socket" };
	}
	auto v = ::recv(m_fd, buf, sz, MSG_NOSIGNAL);
    if (v == -1) {
        throw std::runtime_error { "Read from closed socket" };
    }

    return v;
}

ssize_t TcpSocket::write(const void *buf, size_t sz) {
	if (m_fd == -1) {
		throw std::runtime_error { "Write to closed socket" };
	}
	return ::send(m_fd, buf, sz, 0);
}

TcpServerSocket::TcpServerSocket(uint16_t port)
		: m_port { port } {
	if (!openSocket()) {
		throw std::runtime_error { "Failed to open server socket" };
	}
}

TcpServerSocket::~TcpServerSocket() {
	if (m_fd != -1) {
		shutdown(m_fd, SHUT_RDWR);
		close(m_fd);
	}
}

bool TcpServerSocket::openSocket() {
	m_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (m_fd == -1) {
		perror("socket()");
		return false;
	}

	{
		int opt = 1;
		if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEPORT | SO_REUSEADDR, &opt,
				sizeof(opt)) != 0) {
			perror("setsockopt()");
			close(m_fd);
			m_fd = -1;
			return false;
		}
	}

	memset(&m_sockaddr, 0, sizeof(struct sockaddr_in));

	m_sockaddr.sin_port = htons(m_port);
	m_sockaddr.sin_addr.s_addr = INADDR_ANY;
	m_sockaddr.sin_family = AF_INET;

	if (bind(m_fd, reinterpret_cast<struct sockaddr *>(&m_sockaddr),
			sizeof(struct sockaddr_in)) != 0) {
		perror("bind()");
		close(m_fd);
		m_fd = -1;
		return false;
	}

	if (::listen(m_fd, 5) != 0) {
		perror("listen()");
		close(m_fd);
		m_fd = -1;
		return false;
	}

	return true;
}

bool TcpServerSocket::accept(int &fd, struct sockaddr_in &sa) {
	socklen_t l = sizeof(struct sockaddr_in);
	// TODO: use select

	if ((fd = ::accept(m_fd, reinterpret_cast<struct sockaddr *>(&sa), &l))
			== -1) {
		return false;
	}

	return true;
}
