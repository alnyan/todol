#include "../src/socket.hpp"
#include <signal.h>
#include <iostream>
#include <atomic>

static volatile sig_atomic_t s_running;

void sigint_handler(int signum) {
	s_running = 0;
}

int main(int argc, char **argv) {
	TcpServerSocket serv(12345);
	TcpSocket sock;
	char buf[128];

	signal(SIGINT, sigint_handler);

	s_running = 1;

	while (s_running) {
		if (!serv.listen(sock)) {
			std::cerr << "Listen failed" << std::endl;
			return EXIT_FAILURE;
		}

		auto r = sock.read(buf, 128);
		auto w = sock.write(buf, 128);

		sock.close();
	}

	return EXIT_SUCCESS;
}
