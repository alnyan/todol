#pragma once
#include "todol.hpp"

namespace todol {
namespace at {

int addAtTask(char q, const TaskEntry &task);
std::list<int> listTasks(char q);
bool rmTask(char q, int atId);

}
}
