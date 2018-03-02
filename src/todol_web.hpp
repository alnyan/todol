#pragma once
#include "socket.hpp"
#include <stdint.h>

namespace todol {
namespace web {

class HttpServer {
public:
	HttpServer(uint16_t port);

	void start();

private:

	void serve(TcpSocket &&sock);

	TcpServerSocket m_socket;
};

int startServer(uint16_t port);

}
}
