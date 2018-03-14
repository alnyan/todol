#pragma once
#include "todol_storage.hpp"
#ifdef WITH_COLOR
#include "color.hpp"
#endif
#include "json.hpp"
#include <chrono>
#include <string>
#include <list>

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

using DbHandle = std::unique_ptr<StorageProvider>;
DbHandle open(const std::string &loc);

const char *userHome();
bool fileExists(const std::string &path);

bool readDatabase(DbHandle &j);
bool writeDatabase(DbHandle &j);
void initDatabase(DbHandle &j);

int addTask(DbHandle &db, const std::string &title, uint32_t flags, time_t nt);
bool rmTask(DbHandle &db, int n);
bool setFlags(DbHandle &db, int n, uint32_t flags);
#ifdef WITH_AT
bool addNotify(DbHandle &db, int n, time_t t);
#endif

bool parseIndices(const std::string &idxString, std::list<int> &indices);

int cmdAdd(const std::string &title);
#ifdef WITH_AT
int cmdNotify(int id, const std::string &time, const std::string &date);
#endif
int cmdCat(const std::list<int> &indices, const std::string &c);
int cmdRm(const std::list<int> &indices);
int cmdLs();
int cmdClear();
int cmdDo(const std::list<int> &indices);
int cmdUndo(const std::list<int> &indices);

}
