#ifdef WITH_WEB
#include "todol_web.hpp"
#endif
#ifdef WITH_AT
#include "todol_at.hpp"
#include "timeutil.hpp"
#endif
#include <sys/stat.h>
#include "todol.hpp"
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <climits>
#include <cstring>
#include <pwd.h>

static char s_userHomeBuffer[PATH_MAX + 1] = { 0 };

const char *todol::userHome() {
	if (!s_userHomeBuffer[0]) {
		uid_t uid = getuid();
		struct passwd res;
		struct passwd *resptr;
		char cbuf[16384];

		if (getpwuid_r(uid, &res, cbuf, 16384, &resptr) != 0 || !resptr) {
			return nullptr;
		}

		strcpy(s_userHomeBuffer, resptr->pw_dir);
	}

	return s_userHomeBuffer;
}

bool todol::fileExists(const std::string &path) {
	static struct stat s;
	return stat(path.c_str(), &s) == 0;
}

todol::DbHandle todol::open(const std::string &n) {
    return DbHandle(new JsonStorageProvider(n));
}

int todol::addTask(todol::DbHandle &db, const std::string &title,
		uint32_t flags, time_t t) {
    TaskEntry task;

#ifdef WITH_AT
    task.atId = -1;
#endif
    task.title = title;
    task.timestamp = t;
    task.flags = flags;
    task.id = db->nextId();

    db->save(task);

    return task.id;
}

#ifdef WITH_AT
bool todol::addNotify(todol::DbHandle &db, int n, time_t t) {
    TaskEntry task;

    if (!db->find(n, task)) {
        TODOL_ERROR("Task [" << n << "] does not exist");
        return false;
    }

    if (task.atId != -1) {
        TODOL_ERROR("Task [" << n << "] already has a notification");
        return false;
    }

    task.notifyAt = t;

    int id = todol::at::addAtTask('T', task);

    if (id != -1) {
        task.atId = id;
        db->save(task);
        std::cout << TODOL_COLOR(bold) << TODOL_COLOR(lightgreen)
            << "Added notification for [" << n << "] " << task.title << TODOL_RESET << std::endl;
        return true;
    } else {
        TODOL_ERROR("Failed to create at notification for task [" << n << "]");
        return false;
    }
}
#endif

int todol::cmdAdd(const std::string &title) {
    auto db = todol::open(std::string(userHome()) + "/.todol");

    // TODO: check if entry with title already exists
    time_t t;
    if (!::time(&t)) {
        TODOL_ERROR("Failed to get current time");
        return EXIT_FAILURE;
    }
    int r = addTask(db, title, 0, t);

    if (r == -1) {
        TODOL_ERROR("Failed to add task");
        return EXIT_FAILURE;
    }

    std::cout << TODOL_COLOR(bold) << TODOL_COLOR(lightgreen) << "Added task [" << r << "]"
        << TODOL_RESET << std::endl;

    return EXIT_SUCCESS;
}

int todol::cmdLs() {
    auto db = todol::open(std::string(userHome()) + "/.todol");
    auto ls = db->list();

    char buf[512];

    for (const auto &task : ls) {
        if (task.flags & TODOL_FLAG_COMPLETE) {
            std::cout << TODOL_COLOR(bold) << TODOL_COLOR(lightgreen) << "[" << task.id << "]"
                << TODOL_RESET;
        } else {
            std::cout << "[" << task.id << "]";
        }

        std::cout << "\t" << TODOL_COLOR(bold);
        if (!task.category.empty()) {
            std::cout << "[" << task.category << "] ";
        }
        std::cout << task.title << TODOL_RESET << std::endl;

        struct tm lt;
        localtime_r(&task.timestamp, &lt);
        if (strftime(buf, 512, "%c", &lt) == 0) {
            TODOL_ERROR("Time formatting error");
            return EXIT_FAILURE;
        }

        if (task.flags & TODOL_FLAG_COMPLETE) {
            std::cout << TODOL_COLOR(bold) << TODOL_COLOR(lightgreen) << "+++" << TODOL_RESET;
        }
        std::cout << "\t" << buf << std::endl;
#ifdef WITH_AT
        if (task.atId != -1) {
            localtime_r(&task.notifyAt, &lt);
            if (strftime(buf, 512, "%c", &lt) == 0) {
                TODOL_ERROR("Time formatting error");
                return EXIT_FAILURE;
            }

            time_t ct;
            if (!::time(&ct)) {
                TODOL_ERROR("Failed to get current time");
                return EXIT_FAILURE;
            }

            if (ct > task.notifyAt) {
                if (task.flags & TODOL_FLAG_COMPLETE) {
                    std::cout << TODOL_COLOR(lightblack) << "\tNotified at " << buf << TODOL_RESET
                        << std::endl;
                } else {
                    std::cout << TODOL_COLOR(lightred) << "\tNotified at " << buf
                        << TODOL_RESET << std::endl;
                }
            } else {
                if (!(task.flags & TODOL_FLAG_COMPLETE)) {
                    std::cout << "\tNotify at " << buf << std::endl;
                }
            }
        }
#endif
    }

	return EXIT_SUCCESS;
}

int todol::cmdRm(const std::list<int> &ids) {
    auto db = todol::open(std::string(userHome()) + "/.todol");
    db->remove_if([&ids] (const TaskEntry &e) -> bool {
        bool r = std::find(ids.begin(), ids.end(), e.id) != ids.end();
        if (r) {
#ifdef WITH_AT
            if (e.atId != -1) {
                if (!at::rmTask('T', e.atId)) {
                    TODOL_ERROR("Failed to remove at task for [" << e.id << "]");
                }
            }
#endif
            std::cout << "Removing [" << e.id << "]" << std::endl;
        }
        return r;
    });

    return EXIT_SUCCESS;
}

int todol::cmdClear() {
    auto db = todol::open(std::string(userHome()) + "/.todol");
    db->reset();
    return EXIT_SUCCESS;
}

int todol::cmdDo(const std::list<int> &indices) {
    auto db = todol::open(std::string(userHome()) + "/.todol");

    db->update([&indices] (TaskEntry &task) -> bool {
        bool r = std::find(indices.begin(), indices.end(), task.id) != indices.end();
        if (r && !(task.flags & TODOL_FLAG_COMPLETE)) {
            std::cout << TODOL_COLOR(bold) << TODOL_COLOR(lightgreen) << "Completed [" << task.id
                << "]" << TODOL_RESET << std::endl;
            task.flags |= TODOL_FLAG_COMPLETE;

#ifdef WITH_AT
            if (task.atId != -1) {
                char buf[256];
                struct tm lt;
                time_t t = task.notifyAt;
                localtime_r(&t, &lt);
                size_t l = strftime(buf, 256, "%c", &lt);
                std::cout << "Cancelled notification";
                if (l) {
                    std::cout << " at " << buf;
                }
                std::cout << std::endl;
            }
#endif
        }
        return r;
    });

    return EXIT_SUCCESS;
}

int todol::cmdUndo(const std::list<int> &indices) {
    auto db = todol::open(std::string(userHome()) + "/.todol");

    db->update([&indices] (TaskEntry &task) -> bool {
            bool r = std::find(indices.begin(), indices.end(), task.id) != indices.end();
            if (r && (task.flags & TODOL_FLAG_COMPLETE)) {
                std::cout << "Uncompleted [" << task.id << "]" << std::endl;
            }
            return r;
        });

    return EXIT_SUCCESS;
}

int todol::cmdCat(int n, const std::string &c) {
    auto db = todol::open(std::string(userHome()) + "/.todol");

    db->update([n, &c] (TaskEntry &task) -> bool {
            if (task.id == n) {
                task.category = c;
                return true;
            }
            return false;
        });

    return EXIT_SUCCESS;
}

#ifdef WITH_AT
int todol::cmdNotify(int n, const std::string &time, const std::string &date) {
    auto t = time::parseTime(time, date);
    auto db = todol::open(std::string(userHome()) + "/.todol");

    return addNotify(db, n, t);
}
#endif

bool todol::parseIndices(const std::string &idxString,
		std::list<int> &indices) {
	// TODO: validation
	std::stringstream ss(idxString);
	std::string id;

	while (std::getline(ss, id, ',')) {
		indices.push_back(atoi(id.c_str()));
	}

	return true;
}

