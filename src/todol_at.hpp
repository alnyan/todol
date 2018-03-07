#pragma once
#include "todol.hpp"

namespace todol {
namespace at {

int addAtTask(char q, todol::Task &t);
std::list<int> listTasks(char q);
bool rmTask(char q, todol::Task &t);

}
}
