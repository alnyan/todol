#ifdef WITH_WEB
#include "todol_web.hpp"
#endif
#include "todol.hpp"
#include <iostream>

int main(int argc, char **argv) {
	if (argc < 2) {
		return todol::cmdLs();
	}

	if (!strcmp(argv[1], "ls")) {
		return todol::cmdLs();
	}

	if (!strcmp(argv[1], "add")) {
		if (argc < 3) {
			return EXIT_FAILURE;
		}

		std::string res = argv[2];

		for (int i = 3; i < argc; ++i) {
			res += " " + std::string(argv[i]);
		}

		return todol::cmdAdd(res);
	}

	if (!strcmp(argv[1], "rm")) {
		if (argc != 3) {
			return EXIT_FAILURE;
		}

		std::list<int> ids;

		if (!todol::parseIndices(argv[2], ids) || ids.empty()) {
			return EXIT_FAILURE;
		}

		return todol::cmdRm(ids);
	}

	if (!strcmp(argv[1], "clear")) {
		if (argc != 2) {
			return EXIT_FAILURE;
		}

		return todol::cmdClear();
	}

	if (!strcmp(argv[1], "do")) {
		if (argc != 3) {
			return EXIT_FAILURE;
		}

		std::list<int> ids;

		if (!todol::parseIndices(argv[2], ids) || ids.empty()) {
			return EXIT_FAILURE;
		}

		return todol::cmdDo(ids);
	}

	if (!strcmp(argv[1], "undo")) {
		if (argc != 3) {
			return EXIT_FAILURE;
		}

		std::list<int> ids;

		if (!todol::parseIndices(argv[2], ids) || ids.empty()) {
			return EXIT_FAILURE;
		}

		return todol::cmdUndo(ids);
	}

#ifdef WITH_WEB
	if (!strcmp(argv[1], "serve")) {
		if (argc != 3) {
			return EXIT_FAILURE;
		}

		return todol::web::startServer(atoi(argv[2]));
	}
#endif

	TODOL_ERROR("Unknown command: " << argv[1]);

	return EXIT_FAILURE;
}
