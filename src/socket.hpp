#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include <string>

class TcpSocket {
public:
	TcpSocket(const std::string &dest, uint16_t port);
	TcpSocket();
	TcpSocket(const TcpSocket &other) = delete;
	TcpSocket(TcpSocket &&other);

	~TcpSocket();

	void close();
	void fromDescriptor(const struct sockaddr_in &sa, int fd);

	size_t read(void *buf, size_t sz);
	size_t write(void *buf, size_t sz);

private:

	bool openSocket();

	std::string m_addr;
	uint16_t m_port;
	struct sockaddr_in m_sockaddr;
	int m_fd;
};

class TcpServerSocket {
public:
	TcpServerSocket(uint16_t port);
	~TcpServerSocket();

	bool listen(TcpSocket &sock);

private:

	bool openSocket();

	struct sockaddr_in m_sockaddr;
	uint16_t m_port;
	int m_fd;
};
