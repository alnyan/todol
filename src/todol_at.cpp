#include "todol_at.hpp"
#include <iostream>

#include <unistd.h>
#include <wait.h>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

//std::string exec(const std::string &cmd) {
    //std::array<char, 128> buffer;
    //std::string result;
    //std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
    //if (!pipe) throw std::runtime_error("popen() failed!");
    //while (!feof(pipe.get())) {
        //if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            //result += buffer.data();
    //}
    //return result;
//}
#define READ_END 0
#define WRITE_END 1

static int exec(const std::string &cmd, const std::list<std::string> &args, std::string &result) {
    //std::string result;
    pid_t pid;
    int fds[2];

    if (pipe(fds)) {
        return -1;
    }

    pid = fork();

    if (pid == -1) {
        close(fds[READ_END]);
        close(fds[WRITE_END]);

        return -1;
    }

    if (pid == 0) {
        const char *argv[args.size() + 2];
        argv[0] = cmd.c_str();
        argv[args.size() + 1] = 0;

        size_t i = 1;
        for (const auto &s: args) {
            argv[i++] = s.c_str();
        }

        if (dup2(fds[WRITE_END], STDOUT_FILENO) == -1) {
            perror("dup2(STDOUT)");
            exit(1);
        }
        if (dup2(fds[WRITE_END], STDERR_FILENO) == -1) {
            perror("dup2(STDERR)");
            exit(1);
        }

        close(fds[WRITE_END]);
        close(fds[READ_END]);

        execvp(argv[0], (char *const *) argv);

        exit(1);
    }

    // Write is unused
    close(fds[WRITE_END]);
    int status;
    waitpid(pid, &status, 0);
    char buf[256];
    ssize_t bread;
    do {
        bread = read(fds[READ_END], buf, 256);
        if (bread <= 0) {
            break;
        }
        result += std::string(buf, bread);
    } while (true);

    close(fds[READ_END]);

    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
            std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

static std::string stringifyTimestamp(todol::timestamp_t t) {
    struct tm stm;
    struct tm *stp;
    char buf[4096];

    if (!(stp = localtime_r((time_t *) &t, &stm))) {
        return std::string();
    }

    size_t l = strftime(buf, 4096, "%R %F", stp);

    return std::string(buf, l);
}

static int parseAddedJobId(const std::string &out) {
    std::stringstream ss {out};
    std::string line;

    while (std::getline(ss, line)) {
        const char *p = line.c_str();
        const char *e = strstr(p, "job ");

        if (!e || (e != p)) {
            continue;
        }

        p = strchr(p, ' ') + 1;
        e = strchr(p, ' ');

        return atoi(std::string(p, e - p).c_str());
    }

    return -1;
}

std::list<int> todol::at::listTasks(char q) {
    std::list<int> res;
    std::string x;
    int c = ::exec("atq", {"-q", std::string(1, q)}, x);
    if (c != 0) {
        std::cerr << "atq failed" << std::endl;
    }
    std::stringstream ss(x);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty()) {
            // Parse at -l output here
            const char *p = line.c_str();
            const char *e = strchr(p, '\t');

            if (!e) {
                continue;
            }

            res.push_back(atoi(std::string(p, e - p).c_str()));
        }
    }

    return res;
}


int todol::at::addAtTask(char q, todol::Task &t) {
    int res = -1;
    int tmpfd = -1;
    std::string taskCmd = "notify-send -t 0 \"" + t.title + "\"\n";
    char tmpn[] = "/tmp/todol.tmp.XXXXXX";

    if ((tmpfd = mkstemp(tmpn)) == -1) {
        std::cerr << "Failed to open temporary file" << std::endl;
        t.atId = -1;
        return -1;
    }
    write(tmpfd, taskCmd.c_str(), taskCmd.length());
    close(tmpfd);

    std::string x;
    res = ::exec("at", { "-q", std::string(1, q), "-f", tmpn,
            stringifyTimestamp(t.notifyTime) }, x);

    if (res != 0) {
        std::cerr << "at failed" << std::endl;
        std::cerr << "Commmand: at -q " << q << " -f " << tmpn << stringifyTimestamp(t.notifyTime)
            << std::endl;
        std::cerr << "Output: " << x << std::endl;

        t.atId = -1;
        return -1;
    }

    int jobId = parseAddedJobId(x);
    t.atId = jobId;

    remove(tmpn);

    return jobId;
}

bool todol::at::rmTask(char q, todol::Task &t) {
    int taskAtId = t.atId;

    if (taskAtId == -1) {
        return false;
    }

    t.atId = -1;
    t.notifyTime = 0;
    std::string x;
    int c = ::exec("atrm", {std::to_string(taskAtId)}, x);

    return c == 0;
}
