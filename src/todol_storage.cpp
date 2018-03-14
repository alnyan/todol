#include "todol_storage.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include "json.hpp"

todol::JsonStorageProvider::JsonStorageProvider(const std::string &loc)
    : StorageProvider(loc) {
    readDatabase();
}

todol::JsonStorageProvider::~JsonStorageProvider() {
    if (!cacheValid || !counterValid) {
        flushCache();
    }
}

void todol::JsonStorageProvider::readDatabase() {
    std::string jsonPath = location + "/db.json";
    std::ifstream is(jsonPath);
    cacheValid = true;
    counterValid = true;

    if (!is) {
        counter =0;
        return;
    }

    try {
        nlohmann::json dbJson = nlohmann::json::parse(is);

        counter = dbJson["counter"];
        for (const auto &ent: dbJson["tasks"]) {
            TaskEntry task;

            task.title = ent["title"];
            task.id = ent["id"];
            task.timestamp = ent["timestamp"];
            task.flags = ent["flags"];
#ifdef WITH_AT
            if (ent.find("atId") != ent.end()) {
                task.notifyAt = ent["notifyAt"];
                task.atId = ent["atId"];
            } else {
                task.atId = -1;
            }
#endif
            cache.push_back(task);
        }
    } catch (...) {
        cache = {};
        return;
    }
}

void todol::JsonStorageProvider::flushCache() {
    std::string jsonPath = location + "/db.json";
    std::ofstream os(jsonPath);

    if (!os) {
        return;
    }

    nlohmann::json dbJson;
    dbJson["counter"] = counter;
    dbJson["tasks"] = nlohmann::json::array();
    for (const auto &ent: cache) {
        nlohmann::json taskJson {
            {"title", ent.title},
            {"id", ent.id},
            {"timestamp", ent.timestamp},
            {"flags", ent.flags}
        };

#ifdef WITH_AT
        if (ent.atId != -1) {
            taskJson["atId"] = ent.atId;
            taskJson["notifyAt"] = ent.notifyAt;
        }
#endif

        dbJson["tasks"].push_back(taskJson);
    }

    os << dbJson.dump(4, ' ', 0);
}

std::list<todol::TaskEntry> todol::JsonStorageProvider::list() {
    return cache;
}

bool todol::JsonStorageProvider::save(todol::TaskEntry &entry) {
    // Store in cache
    for (auto &t: cache) {
        if (t.id == entry.id) {
            t = entry;
            cacheValid = false;
            return true;
        }
    }

    cache.push_back(entry);
    cacheValid = false;

    return true;
}

bool todol::JsonStorageProvider::remove(int id) {
    auto it = std::remove_if(cache.begin(), cache.end(), [id](const TaskEntry &e) {
                return e.id == id;
            });
    bool r = it != cache.end();
    cache.erase(it, cache.end());
    return r;
}

void todol::JsonStorageProvider::remove_if(
        const std::function<bool(const todol::TaskEntry &)> &pred) {
    auto it = std::remove_if(cache.begin(), cache.end(), pred);
    bool r = it != cache.end();
    cache.erase(it, cache.end());
    if (r) {
        cacheValid = false;
    }
}

bool todol::JsonStorageProvider::find(int id, todol::TaskEntry &entry) {
    auto it = std::find_if(cache.begin(), cache.end(), [id](const TaskEntry &e) {
            return e.id == id;
        });

    if (it != cache.end()) {
        entry = *it;
        return true;
    } else {
        return false;
    }
}

void todol::JsonStorageProvider::update(const std::function<bool(todol::TaskEntry &)> &mut) {
    for (auto &task: cache) {
        if (mut(task)) {
            cacheValid = false;
        }
    }
}

int todol::JsonStorageProvider::nextId() {
    counterValid = false;
    return counter++;
}

bool todol::JsonStorageProvider::reset() {
    cache = {};
    cacheValid = false;
    counter = 0;
    counterValid = false;
    return true;
}
