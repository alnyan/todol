#include "timeutil.hpp"
#include <string.h>

time_t todol::time::parseTime(const std::string &time, const std::string &in) {
    // Process time here
    int hours = -1, minutes = -1;
    int cdays = 0;
    {
        if (strchr(time.c_str(), ':') && time[0] != '+') {
            struct tm _stm;
            strptime(time.c_str(), "%R", &_stm);

            hours = _stm.tm_hour;
            minutes = _stm.tm_min;
        } else {
            const char *p = time.c_str();
            const char *e = strchr(p, ':');
            if (p[0] == '+' && e) {
                struct tm _stm;
                struct tm *_stp;
                time_t _t;
                if (!::time(&_t)) {
                    return -1;
                }

                if (!(_stp = localtime_r(&_t, &_stm))) {
                    return -1;
                }

                hours = _stp->tm_hour;
                minutes = _stp->tm_min;

                int ah = atoi(std::string(p, e - p).c_str());
                int am = atoi(std::string(e + 1).c_str());

                hours += ah;
                minutes += am;

                // TODO: optimize
                while (minutes >= 60) {
                    minutes -= 60;
                    ++hours;
                }

                while (hours >= 24) {
                    hours -= 24;
                    ++cdays;
                }
            } else if (p[0] == '+') {
                struct tm _stm;
                struct tm *_stp;
                time_t _t;
                if (!::time(&_t)) {
                    return -1;
                }

                if (!(_stp = localtime_r(&_t, &_stm))) {
                    return -1;
                }

                hours = _stp->tm_hour;
                minutes = _stp->tm_min;

                int ah = atoi(p);

                hours += ah;

                // TODO: optimize
                while (hours >= 24) {
                    hours -= 24;
                    ++cdays;
                }
            } else {
                return -1;
            }
        }
    }

    if (hours == -1 || minutes == -1) {
        return -1;
    }

    // Process date here
    time_t t;
    char buf[255];
    struct tm stm;
    struct tm *stp;

    if (!::time(&t)) {
        return {};
    }

    t += cdays * 3600 * 24;

    stp = localtime_r(&t, &stm);

    if (!stp) {
        return -1;
    }

    if (in == "Tomorrow" || in == "tomorrow") {
        t += 3600 * 24;

        stp = localtime_r(&t, &stm);
    } else if (!in.empty() && (in[0] == '+')) {
        const char *p = in.c_str() + 1;

        if (!*p) {
            return -1;
        }

        t += 3600 * 24 * atoi(p);

        stp = localtime_r(&t, &stm);
    } else if (in.empty()) {
        char _datebuf[255];
        stp->tm_hour = hours;
        stp->tm_min = minutes;

        if (!cdays) {
            time_t _t = mktime(stp);

            if (t > _t) {
                t += 3600 * 24;
            }
        }

        stp = localtime_r(&t, &stm);
    } else {
        strptime(in.c_str(), "%F", &stm);
        stp = &stm;
    }

    stp->tm_hour = hours;
    stp->tm_min = minutes;

    //size_t sz = strftime(buf, 255, "%R %F", &stm);

    return mktime(stp);
}

