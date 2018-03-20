#include "todol_dmenu.hpp"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#define READ_END    0
#define WRITE_END   1

static std::string rtrim(const std::string &in) {
    size_t l = in.length();
    size_t x = l;
    while (l--) {
        if (!isspace(in[l])) {
            break;
        }
    }
    if (l < x - 1) {
        return std::string(in.c_str(), l + 1);
    } else {
        return in;
    }
}

std::string todol::dmenu_ask(const std::list<std::string> &choices, const std::string &e) {
    pid_t pid;
    int in_fds[2];
    int out_fds[2];

    if (pipe(in_fds) < 0) {
        return "";
    }

    if (pipe(out_fds) < 0) {
        close(in_fds[0]);
        close(in_fds[1]);
        return "";
    }

    if ((pid = fork()) < 0) {
        close(in_fds[READ_END]);
        close(in_fds[WRITE_END]);
        close(out_fds[READ_END]);
        close(out_fds[WRITE_END]);
        return "";
    }

    if (pid) {
        close(in_fds[WRITE_END]);
        close(out_fds[READ_END]);

        dup2(in_fds[READ_END], STDIN_FILENO);
        dup2(out_fds[WRITE_END], STDOUT_FILENO);

        char *const argv[2] = {strdup(e.c_str()), NULL};

        execvp(e.c_str(), argv);

        exit(-1);
    } else {
        close(in_fds[READ_END]);
        close(out_fds[WRITE_END]);

        char nl = '\n';
        for (const auto &line: choices) {
            write(in_fds[WRITE_END], line.c_str(), line.size());
            write(in_fds[WRITE_END], &nl, 1);
        }

        close(in_fds[WRITE_END]);

        char buf[256];
        ssize_t bread;
        bread = read(out_fds[READ_END], buf, 255);

        close(out_fds[READ_END]);

        if (bread < 0) {
            return "";
        }

        return rtrim(std::string(buf, bread));
    }
}

