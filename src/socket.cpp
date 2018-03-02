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

TcpSocket::TcpSocket(TcpSocket &&other)
		: m_fd { other.m_fd }, m_addr { other.m_addr }, m_port { other.m_port }, m_sockaddr {
				other.m_sockaddr } {
	std::cout << "Move " << this << std::endl;
}

TcpSocket::TcpSocket()
		: m_fd { -1 }, m_addr { "" }, m_port { 0 } {
}

TcpSocket::~TcpSocket() {
	std::cout << "Destructor " << this << std::endl;

	if (m_fd != -1) {
		shutdown(m_fd, SHUT_RDWR);
		::close(m_fd);
	}
}

void TcpSocket::close() {
	std::cout << "Close " << this << std::endl;

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

size_t TcpSocket::read(void *buf, size_t sz) {
	return ::recv(m_fd, buf, sz, 0);
}

size_t TcpSocket::write(void *buf, size_t sz) {
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

bool TcpServerSocket::listen(TcpSocket &sock) {
	struct sockaddr_in sa;
	socklen_t l = sizeof(struct sockaddr_in);
	int fd = -1;
	// TODO: use select

	if ((fd = accept(m_fd, reinterpret_cast<struct sockaddr *>(&sa), &l))
			== -1) {
		return false;
	}

	sock.fromDescriptor(sa, fd);

	return true;
}
