#ifdef WITH_WEB
#include "todol_web.hpp"
#endif
#include "todol.hpp"
#include <iostream>

static const std::string s_helpMessage =
    "Usage: todol <command> ...\n"
    "Possible commands:\n"
    "\t* ls (or nothing) - outputs list of tasks\n"
    "\t* add <title> - adds a task to the list\n"
    "\t* rm <N> - removes task N from the list\n"
    "\t* clear - removes all tasks\n"
    "\t* do <N> - marks task N as completed\n"
    "\t* undo <N> - removes complete flag from task N\n"
    "\t* cat <N> <category> - sets N's category (or clears, if none specified)\n"
    "\t* help - prints this message\n"
#ifdef WITH_AT
    "\t* notify <N> <TIME> [DATE] - adds a notification for task N\n"
#endif
    ;

int printHelp() {
    std::cerr << s_helpMessage;
    return EXIT_SUCCESS;
}

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

    if (!strcmp(argv[1], "help") || !strcmp(argv[1], "--help")) {
        return printHelp();
    }

    if (!strcmp(argv[1], "cat")) {
        if (argc < 3 || argc > 4) {
            return EXIT_FAILURE;
        }
        std::string cat;
        if (argc == 4) {
            cat = argv[3];
        }

        std::list<int> ids;

        if (!todol::parseIndices(argv[2], ids) || ids.empty()) {
            return EXIT_FAILURE;
        }

        return todol::cmdCat(ids, cat);
    }

#ifdef WITH_WEB
    if (!strcmp(argv[1], "serve")) {
        if (argc != 3) {
            return EXIT_FAILURE;
        }

        return todol::web::startServer(atoi(argv[2]));
    }
#endif

#ifdef WITH_AT
    if (!strcmp(argv[1], "notify")) {
        switch (argc) {
            case 4:
                return todol::cmdNotify(atoi(argv[2]), argv[3], "");
            case 5:
                return todol::cmdNotify(atoi(argv[2]), argv[3], argv[4]);
            default:
                return EXIT_FAILURE;
        }
    }
#endif
    TODOL_ERROR("Unknown command: " << argv[1]);

	return EXIT_FAILURE;
}
