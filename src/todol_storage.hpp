#pragma once
#include <functional>
#include <string>
#include <list>

namespace todol {

struct TaskEntry {
    int id;
    std::string title;
    uint32_t flags;
    time_t timestamp;
#ifdef WITH_AT
    time_t notifyAt;
    int atId;
#endif
};

class StorageProvider {
public:
    StorageProvider(const std::string &loc): location{loc} {
    }
    virtual ~StorageProvider() {
    }

    virtual void update(const std::function<bool(TaskEntry &)> &mut) = 0;
    virtual bool reset() = 0;
    virtual std::list<TaskEntry> list() = 0;
    virtual bool save(TaskEntry &entry) = 0;
    virtual bool remove(int id) = 0;
    virtual void remove_if(const std::function<bool(const TaskEntry &)> &pred) = 0;
    virtual bool find(int id, TaskEntry &entry) = 0;
    virtual int nextId() = 0;

protected:

    const std::string location;
};

class JsonStorageProvider: public StorageProvider {
public:
    JsonStorageProvider(const std::string &location);
    ~JsonStorageProvider();

    void update(const std::function<bool(TaskEntry &)> &mut) override;
    bool reset() override;
    std::list<TaskEntry> list() override;
    bool save(TaskEntry &entry) override;
    bool remove(int id) override;
    void remove_if(const std::function<bool(const TaskEntry &)> &pred) override;
    bool find(int id, TaskEntry &entry) override;
    int nextId() override;

private:

    void readDatabase();
    void flushCache();

    std::list<TaskEntry> cache;
    bool cacheValid, counterValid;
    int counter;
};

}
