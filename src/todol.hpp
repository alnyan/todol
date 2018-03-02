#pragma once
#ifdef WITH_COLOR
#include "color.hpp"
#endif
#include "json.hpp"
#include <chrono>
#include <string>
#include <list>

using njson = nlohmann::json;

#define TODOL_FLAG_COMPLETE 1

#ifdef WITH_COLOR
#define TODOL_COLOR(n) todol::color::n
#define TODOL_RESET todol::color::reset

#define TODOL_ERROR(m) std::cerr << todol::color::bold << todol::color::red << m << todol::color::reset << std::endl
#else
#define TODOL_COLOR(n) ""
#define TODOL_RESET    ""
#define TODOL_ERROR(m) std::cerr << m << std::endl
#endif

namespace todol {

using timestamp_t = uint64_t;
using flags_t = uint32_t;
using id_t = uint32_t;

struct Task {
	std::string title;
	timestamp_t timestamp;
	flags_t flags;
	id_t id;
};

struct DbHandle {
	njson json;
};

const char *userHome();
bool fileExists(const std::string &path);

bool readDatabase(DbHandle &j);
bool writeDatabase(DbHandle &j);
void initDatabase(DbHandle &j);

int addTask(DbHandle &db, const std::string &title, uint32_t flags);
bool rmTask(DbHandle &db, int n);
void setFlags(DbHandle &db, int n, uint32_t flags);
int findTask(const DbHandle &db, const std::string &title, Task &t);
bool getTask(const DbHandle &db, int n, Task &t);
std::list<Task> lsTasks(DbHandle &db);

bool parseIndices(const std::string &idxString, std::list<int> &indices);

int cmdAdd(const std::string &title);
int cmdRm(const std::list<int> &indices);
int cmdLs();
int cmdClear();
int cmdDo(const std::list<int> &indices);
int cmdUndo(const std::list<int> &indices);

}
