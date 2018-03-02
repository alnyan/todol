#include "../src/todol_web.hpp"

int main(int argc, char **argv) {
	todol::web::HttpServer serv(12345);
	serv.start();
	return EXIT_SUCCESS;
}
