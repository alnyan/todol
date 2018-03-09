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

bool todol::readDatabase(todol::DbHandle &j) {
	std::ifstream s(std::string(userHome()) + "/.todol/db.json");

	if (!s) {
		return false;
	}

	j.json = nlohmann::json::parse(s);

	return true;
}

bool todol::writeDatabase(todol::DbHandle &j) {
	std::string p = userHome();
	p += "/.todol";

	if (!fileExists(p)) {
		if (mkdir(p.c_str(), 0744) != 0) {
			return false;
		}
	}

	p += "/db.json";

	std::ofstream s(p);

	if (!s) {
		return false;
	}

	s << j.json.dump(4, ' ', false);

	return true;
}

void todol::initDatabase(todol::DbHandle &j) {
	j.json["counter"] = 0;
}

int todol::addTask(todol::DbHandle &db, const std::string &title,
		uint32_t flags, timestamp_t t) {
	if (!db.json["tasks"].is_array()) {
		db.json["tasks"] = njson::array();
	}

	int id = db.json["counter"];
	db.json["counter"] = id + 1;

    njson task;
    task["title"] = title;
    task["timestamp"] = ::time(nullptr);
    task["flags"] = flags;
    task["id"] = id;

	db.json["tasks"].push_back(task);

	return id;
}

#ifdef WITH_AT
bool todol::addNotify(todol::DbHandle &db, int n, timestamp_t t) {
    for (auto &ent: db.json["tasks"]) {
        if (ent["id"] == n) {
            if (ent.find("atId") != ent.end()) {
                TODOL_ERROR("Task [" << n << "] already has notification");
                return false;
            }

            ent["notifyTime"] = t;
            int id = todol::at::addAtTask('T', ent);

            if (id != -1) {
                std::cout << TODOL_COLOR(bold) << TODOL_COLOR(lightgreen)
                    << "Added notification for [" << id << "] " << ent["title"].get<std::string>()
                    << TODOL_RESET << std::endl;
                return true;
            }

            return false;
        }
    }

    TODOL_ERROR("Task [" << n << "] does not exist");
    return false;
}
#endif

int todol::cmdAdd(const std::string &title) {
	DbHandle db;

	if (readDatabase(db)) {
	    auto it = std::find_if(db.json["tasks"].begin(), db.json["tasks"].end(),
                [&title](const njson &task) -> bool {
                    return task["title"] == title;
                });

        if (it != db.json["tasks"].end()) {
            TODOL_ERROR("Task with such title already exists");
            return EXIT_FAILURE;
        }
	} else {
		initDatabase(db);
	}

	int id = addTask(db, title, 0, 0);

	std::cout << TODOL_COLOR(bold) << TODOL_COLOR(lightgreen) << "Added [" << id
			<< "]" << TODOL_RESET << std::endl;

	if (!writeDatabase(db)) {
		TODOL_ERROR("Failed to save task");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int todol::cmdLs() {
	DbHandle db;

	if (!readDatabase(db)) {
		TODOL_ERROR("Database does not exist");
		return EXIT_FAILURE;
	}

	char buf[512];

	for (const auto &task : db.json["tasks"]) {
		if (static_cast<flags_t>(task["flags"]) & TODOL_FLAG_COMPLETE) {
			std::cout << TODOL_COLOR(bold) << TODOL_COLOR(lightgreen) << "["
					<< task["id"] << "]" << TODOL_RESET;
		} else {
			std::cout << "[" << task["id"] << "]";
		}
		std::cout << "\t" << TODOL_COLOR(bold) << task["title"].get<std::string>()
            << TODOL_RESET << std::endl;
		time_t t = task["timestamp"];
		struct tm lt;
		localtime_r(&t, &lt);
		if (strftime(buf, 512, "%c", &lt) == 0) {
		    TODOL_ERROR("Time formatting error");
            return EXIT_FAILURE;
		}
		if (static_cast<flags_t>(task["flags"]) & TODOL_FLAG_COMPLETE) {
			std::cout << TODOL_COLOR(bold) << TODOL_COLOR(lightgreen) << "+++"
					<< TODOL_RESET;
		}
		std::cout << "\t" << buf << std::endl;
#ifdef WITH_AT
        if (task.find("notifyTime") != task.end()) {
            t = task["notifyTime"];
            localtime_r(&t, &lt);
            if (strftime(buf, 512, "%c", &lt) == 0) {
                TODOL_ERROR("Time formatting error");
                return EXIT_FAILURE;
            }

            time_t ct;
            if (!::time(&ct)) {
                TODOL_ERROR("Failed to get current time");
                return EXIT_FAILURE;
            }

            if (ct > t) {
                if (static_cast<flags_t>(task["flags"]) & TODOL_FLAG_COMPLETE) {
                    std::cout << TODOL_COLOR(lightblack) << "\tNotified at " << buf << TODOL_RESET
                        << std::endl;
                } else {
                    std::cout << TODOL_COLOR(lightred) << "\tNotified at " << buf
                        << TODOL_RESET << std::endl;
                }
            } else {
                std::cout << "\tNotify at " << buf << std::endl;
            }
        }
#endif
	}

	return EXIT_SUCCESS;
}

int todol::cmdRm(const std::list<int> &ids) {
	DbHandle db;

	if (!readDatabase(db)) {
		TODOL_ERROR("Database does not exist");
		return EXIT_FAILURE;
	}

	auto oldSize = db.json["tasks"].size();

	db.json["tasks"].erase(
			std::remove_if(db.json["tasks"].begin(), db.json["tasks"].end(),
					[&ids](const njson &t) -> bool {
						int id = t["id"];
						bool r = std::find(ids.begin(), ids.end(), id) != ids.end();
						if (r) {
							std::cout << "Removing [" << id << "] "
                                << t["title"].get<std::string>() << std::endl;
#ifdef WITH_AT
                                if (t.find("atId") != t.end()) {
                                    if (!at::rmTask('T', t["atId"])) {
                                        TODOL_ERROR("Failed to remove at task");
                                    }
                                }
#endif
						}
						return r;
					}), db.json["tasks"].end());

	if (!writeDatabase(db)) {
		TODOL_ERROR("Failed to commit changes");
		return EXIT_FAILURE;
	}

	return oldSize == db.json["tasks"].size() ? EXIT_FAILURE : EXIT_SUCCESS;
}

int todol::cmdClear() {
	DbHandle db;

	if (!readDatabase(db)) {
		TODOL_ERROR("Database does not exist");
		return EXIT_FAILURE;
	}

	db.json["tasks"] = njson::array();
	db.json["counter"] = 0;

	if (!writeDatabase(db)) {
		TODOL_ERROR("Failed to commit changes");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int todol::cmdDo(const std::list<int> &indices) {
	DbHandle db;

	if (!readDatabase(db)) {
		TODOL_ERROR("Database does not exist");
		return EXIT_FAILURE;
	}

	for (auto &task : db.json["tasks"]) {
		bool r = std::find(indices.begin(), indices.end(), static_cast<id_t>(task["id"]))
				!= indices.end();

		if (r) {
			if (static_cast<flags_t>(task["flags"]) & TODOL_FLAG_COMPLETE) {
				std::cout << TODOL_COLOR(bold) << TODOL_COLOR(red) << "["
						<< task["id"] << "] `" << task["title"].get<std::string>()
                        << "' Is already completed" << TODOL_RESET << std::endl;
			} else {
				task["flags"] = static_cast<flags_t>(task["flags"]) | TODOL_FLAG_COMPLETE;
				std::cout << TODOL_COLOR(bold) << TODOL_COLOR(lightgreen)
						<< "Completed " << "[" << task["id"] << "] "
                        << task["title"].get<std::string>()	<< TODOL_RESET << std::endl;
			}
		}
	}

	if (!writeDatabase(db)) {
		TODOL_ERROR("Failed to commit changes");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int todol::cmdUndo(const std::list<int> &indices) {
	DbHandle db;

	if (!readDatabase(db)) {
		TODOL_ERROR("Database does not exist");
		return EXIT_FAILURE;
	}

	for (auto &task : db.json["tasks"]) {
		bool r = std::find(indices.begin(), indices.end(), static_cast<id_t>(task["id"]))
				!= indices.end();

		if (r) {
			if (static_cast<flags_t>(task["flags"]) & TODOL_FLAG_COMPLETE) {
				task["flags"] =
						static_cast<flags_t>(task["flags"]) & ~TODOL_FLAG_COMPLETE;
				std::cout << TODOL_COLOR(bold) << "Uncompleted [" << task["id"]
						<< "] " << task["title"].get<std::string>() << TODOL_RESET << std::endl;
			} else {
				std::cout << TODOL_COLOR(bold) << TODOL_COLOR(red) << "["
						<< task["id"] << "] `" << task["title"].get<std::string>()
                        << "' Is not completed" << TODOL_RESET << std::endl;
			}
		}
	}

	if (!writeDatabase(db)) {
		TODOL_ERROR("Failed to commit changes");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

#ifdef WITH_AT
int todol::cmdNotify(int n, const std::string &time, const std::string &date) {
    auto t = time::parseTime(time, date);

    DbHandle db;

    if (!readDatabase(db)) {
        TODOL_ERROR("Database does not exist");
        return EXIT_FAILURE;
    }

    if (!addNotify(db, n, static_cast<timestamp_t>(t))) {
        TODOL_ERROR("Failed to add notify to [" << n << "]");
        return EXIT_FAILURE;
    }

    if (!writeDatabase(db)) {
        TODOL_ERROR("Failed to commit changes");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
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
