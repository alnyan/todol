#include "todol_web.hpp"
#include "todol.hpp"
#include <iostream>
#include <thread>

todol::web::HttpServer::HttpServer(uint16_t port)
		: m_socket(port) {
}

void todol::web::HttpServer::start() {
	while (true) {
		TcpSocket clientSocket;

		if (m_socket.listen(clientSocket)) {
			std::cout << "Pre-bind" << std::endl;
			auto fn = std::bind(&HttpServer::serve, this,
					std::placeholders::_1);
			std::cout << "Post-bind" << std::endl;

			std::thread thr(fn, std::move(clientSocket));

			std::cout << "Pre-detach" << std::endl;
			thr.detach();
		}
	}
}

void todol::web::HttpServer::serve(TcpSocket &&socket) {
}

int todol::web::startServer(uint16_t port) {
	std::cout << "Starting server on " << port << "..." << std::endl;
	std::cout << "Press Ctrl+C to terminate" << std::endl;

	HttpServer server(port);
	server.start();

	return EXIT_SUCCESS;
}
