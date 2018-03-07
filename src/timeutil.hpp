#pragma once
#include <time.h>
#include <string>

namespace todol {
namespace time {

time_t parseTime(const std::string &time, const std::string &date);

}
}
