#include "../src/todol_web.hpp"

int main(int argc, char **argv) {
	todol::web::startServer(12345);
	return EXIT_SUCCESS;
}
