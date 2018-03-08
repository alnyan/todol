#include "../src/todol_at.hpp"
#include <iostream>

int main(int argc, char **argv) {
    todol::Task t;
    t.id = 1;
    t.title = "Something!";
    t.notifyTime = static_cast<todol::timestamp_t>(time(NULL) + 61);

    int atid = todol::at::addAtTask('T', t);

    return 0;
}
