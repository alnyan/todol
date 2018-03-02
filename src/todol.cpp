#include <sys/stat.h>
#include "todol.hpp"
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <climits>
#include <cstring>
#include <pwd.h>
#include "todol_web.hpp"

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
		uint32_t flags) {
	if (!db.json["tasks"].is_array()) {
		db.json["tasks"] = njson::array();
	}

	int id = db.json["counter"];
	db.json["counter"] = id + 1;

	db.json["tasks"].push_back(
			njson { { "title", title }, { "timestamp", time(nullptr) }, {
					"flags",
					flags }, { "id", id } });

	return id;
}

int todol::findTask(const todol::DbHandle &db, const std::string &title,
		todol::Task &t) {
	return -1;
}

std::list<todol::Task> todol::lsTasks(todol::DbHandle &db) {
	std::list<Task> res;

	for (const auto &ent : db.json["tasks"]) {
		res.push_back(
				{ ent["title"], ent["timestamp"], ent["flags"], ent["id"] });
	}

	return res;
}

int todol::cmdAdd(const std::string &title) {
	Task t;
	DbHandle db;

	if (readDatabase(db)) {
		if (findTask(db, title, t) != -1) {
			std::cerr << "Task \"" << title << "\" already exists" << std::endl;
			return EXIT_FAILURE;
		}
	} else {
		initDatabase(db);
	}

	int id = addTask(db, title, 0);

	std::cout << "Added " << id << std::endl;

	if (!writeDatabase(db)) {
		std::cerr << "Failed to save task" << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int todol::cmdLs() {
	DbHandle db;

	if (!readDatabase(db)) {
		std::cerr << "Database does not exist" << std::endl;
		return EXIT_FAILURE;
	}

	auto ls = lsTasks(db);
	char buf[512];

	for (const auto &task : ls) {
		std::cout << "[" << task.id << "]";
		if (task.flags & TODOL_FLAG_COMPLETE) {
			std::cout << "+++";
		}
		std::cout << "\t" << task.title << std::endl;
		time_t t = task.timestamp;
		struct tm lt;
		localtime_r(&t, &lt);
		if (strftime(buf, 512, "%c", &lt) == 0) {
			std::cerr << "Error happened" << std::endl;
			return EXIT_FAILURE;
		}
		std::cout << "\t" << buf << std::endl;
	}

	return EXIT_SUCCESS;
}

int todol::cmdRm(const std::list<int> &ids) {
	DbHandle db;

	if (!readDatabase(db)) {
		std::cerr << "Database does not exist" << std::endl;
		return EXIT_FAILURE;
	}

	auto oldSize = db.json["tasks"].size();

	db.json["tasks"].erase(
			std::remove_if(db.json["tasks"].begin(), db.json["tasks"].end(),
					[&ids](const njson &t) -> bool {
						int id = t["id"];
						bool r = std::find(ids.begin(), ids.end(), id) != ids.end();
						if (r) {
							std::cout << "Removing [" << id << "] " << t["title"] << std::endl;
						}
						return r;
					}), db.json["tasks"].end());

	if (!writeDatabase(db)) {
		std::cerr << "Failed to commit changes" << std::endl;
		return EXIT_FAILURE;
	}

	return oldSize == db.json["tasks"].size() ? EXIT_FAILURE : EXIT_SUCCESS;
}

int todol::cmdClear() {
	DbHandle db;

	if (!readDatabase(db)) {
		std::cerr << "Database does not exist" << std::endl;
		return EXIT_FAILURE;
	}

	db.json["tasks"] = njson::array();
	db.json["counter"] = 0;

	if (!writeDatabase(db)) {
		std::cerr << "Failed to commit changes" << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int todol::cmdDo(const std::list<int> &indices) {
	DbHandle db;

	if (!readDatabase(db)) {
		std::cerr << "Database does not exist" << std::endl;
		return EXIT_FAILURE;
	}

	for (auto &task : db.json["tasks"]) {
		bool r = std::find(indices.begin(), indices.end(), (int) task["id"])
				!= indices.end();

		if (r) {
			if (((flags_t) task["flags"]) & TODOL_FLAG_COMPLETE) {
				std::cout << "[" << task["id"] << "] Is already completed"
						<< std::endl;
			} else {
				task["flags"] = ((flags_t) task["flags"]) | TODOL_FLAG_COMPLETE;
				std::cout << "Completed [" << task["id"] << "]" << std::endl;
			}
		}
	}

	if (!writeDatabase(db)) {
		std::cerr << "Failed to commit changes" << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int todol::cmdUndo(const std::list<int> &indices) {
	DbHandle db;

	if (!readDatabase(db)) {
		std::cerr << "Database does not exist" << std::endl;
		return EXIT_FAILURE;
	}

	for (auto &task : db.json["tasks"]) {
		bool r = std::find(indices.begin(), indices.end(), (int) task["id"])
				!= indices.end();

		if (r) {
			if (((flags_t) task["flags"]) & TODOL_FLAG_COMPLETE) {
				task["flags"] =
						((flags_t) task["flags"]) & ~TODOL_FLAG_COMPLETE;
				std::cout << "Uncompleted [" << task["id"] << "]" << std::endl;
			} else {
				std::cout << "[" << task["id"] << "] Is not completed"
						<< std::endl;
			}
		}
	}

	if (!writeDatabase(db)) {
		std::cerr << "Failed to commit changes" << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

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
