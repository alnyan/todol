#pragma once
#include "todol.hpp"

namespace todol {
namespace at {

int addAtTask(char q, njson &task);
std::list<int> listTasks(char q);
bool rmTask(char q, njson &t);

}
}
