#ifndef COMMON_HPP
#define COMMON_HPP

#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace log2what {
using std::string;
enum class level : int {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR
};

string to_string(level l) {
    switch (l) {
    case level::TRACE:
        return "TRACE";
    case level::DEBUG:
        return "DEBUG";
    case level::INFO:
        return "INFO ";
    case level::WARN:
        return "WARN ";
    case level::ERROR:
        return "ERROR";
    default:
        return "LEVEL";
    }
}

void to_string(string &s, level l) {
    s = to_string(l);
}

template <typename T>
int64_t get_timestamp() {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<T>(now).count();
}

int64_t get_nano_timestamp() {
    return get_timestamp<std::chrono::nanoseconds>();
}

namespace time_const {
constexpr int EPOCH_TO_2M = 946656000;
constexpr int MINUTE = 60;
constexpr int HOUR = MINUTE * 60;
constexpr int DAY = HOUR * 24;
constexpr int YEAR = DAY * 365;
constexpr int64_t FOUR_YEAR = DAY * (365 * 4 + 1);
constexpr int64_t FOUR_HUNDRED_YEAR = FOUR_YEAR * 100 - DAY * 3;
} // namespace time_const

struct datetime {
    int year;
    int month;
    int day;
    int zone;
    int weekday;
    int hour;
    int mintue;
    int second;
    int nano;
    string to_string() const {
        using namespace std;
        stringstream ss;
        ss << year
           << "-" << setfill('0') << setw(2) << month
           << "-" << setfill('0') << setw(2) << day
           << "T" << setfill('0') << setw(2) << hour
           << ":" << setfill('0') << setw(2) << mintue
           << ":" << fixed << setfill('0') << setw(6)
           << setprecision(3) << nano / 1e9 + second;
        if (zone) {
            ss << setiosflags(ios::showpos) << zone;
        } else {
            ss << "Z";
        }
        ss << resetiosflags(ios::showpos) << " " << weekday;
        return ss.str();
    }
    operator string() {
        return to_string();
    }
    friend std::ostream &operator<<(std::ostream &out, const datetime &dt) {
        out << dt.to_string();
        return out;
    }
};

datetime get_datetime_after(datetime &dt, int64_t ts_sec) {
    using namespace time_const;
    constexpr std::array<int, 4> YEAR_DAYS_ARRAY{YEAR, YEAR, YEAR + DAY, YEAR};
    constexpr std::array<int, 24> MONTH_DAYS_ARRAY{
        31 * DAY, 31 * DAY, 28 * DAY, 29 * DAY,
        31 * DAY, 31 * DAY, 30 * DAY, 30 * DAY,
        31 * DAY, 31 * DAY, 30 * DAY, 30 * DAY,
        31 * DAY, 31 * DAY, 31 * DAY, 31 * DAY,
        30 * DAY, 30 * DAY, 31 * DAY, 31 * DAY,
        30 * DAY, 30 * DAY, 31 * DAY, 31 * DAY};
    constexpr std::array<int, 14> WEEKDAY_ARRAY{4, 5, 6, 7, 1, 2, 3};
    dt.weekday = WEEKDAY_ARRAY[((ts_sec / DAY) % 7)];
    // decode year
    dt.year = 1970;
    dt.year += (ts_sec / FOUR_HUNDRED_YEAR) * 400;
    ts_sec %= FOUR_HUNDRED_YEAR;
    dt.year += (ts_sec / FOUR_YEAR) * 4;
    ts_sec %= FOUR_YEAR;
    int is_leap = 0;
    for (auto &&y : YEAR_DAYS_ARRAY) {
        if (ts_sec >= y) {
            ts_sec -= y;
            dt.year += 1;
        } else {
            is_leap = (y != YEAR) ? 1 : 0;
            break;
        }
    }
    // decode month
    dt.month = 1;
    for (size_t i = 0; i < 12; i++) {
        int month = MONTH_DAYS_ARRAY[2 * i + is_leap];
        if (ts_sec >= month) {
            ts_sec -= month;
        } else {
            dt.month += i;
            break;
        }
    }
    // decode left
    dt.day = 1;
    dt.day += ts_sec / DAY;
    ts_sec %= DAY;
    dt.hour = ts_sec / HOUR;
    ts_sec %= HOUR;
    dt.mintue = ts_sec / MINUTE;
    dt.second = ts_sec % MINUTE;
    return dt;
}

// 1950-05-04T01:02:03+8 thur
datetime get_datetime_before(datetime &dt, int64_t ts_sec) {
    using namespace time_const;
    constexpr std::array<int, 4> YEAR_DAYS_ARRAY{YEAR, YEAR + DAY, YEAR, YEAR};
    constexpr std::array<int, 24> MONTH_DAYS_ARRAY{
        31 * DAY, 31 * DAY, 28 * DAY, 29 * DAY,
        31 * DAY, 31 * DAY, 30 * DAY, 30 * DAY,
        31 * DAY, 31 * DAY, 30 * DAY, 30 * DAY,
        31 * DAY, 31 * DAY, 31 * DAY, 31 * DAY,
        30 * DAY, 30 * DAY, 31 * DAY, 31 * DAY,
        30 * DAY, 30 * DAY, 31 * DAY, 31 * DAY};
    constexpr std::array<int, 14> WEEKDAY_ARRAY{3, 2, 1, 7, 6, 5, 4};
    dt.weekday = WEEKDAY_ARRAY[((ts_sec / DAY) % 7)];
    // decode year 126,230,400
    dt.year = 1969;
    dt.year -= (ts_sec / FOUR_HUNDRED_YEAR) * 400;
    ts_sec %= FOUR_HUNDRED_YEAR;
    dt.year -= (ts_sec / FOUR_YEAR) * 4;
    ts_sec %= FOUR_YEAR;
    int is_leap = 0;
    for (auto &&y : YEAR_DAYS_ARRAY) {
        if (ts_sec >= y) {
            ts_sec -= y;
            dt.year -= 1;
        } else {
            ts_sec = y - ts_sec;
            is_leap = (y != YEAR);
            break;
        }
    }
    // 重新定位到该年年初
    std::cout << ts_sec << std::endl;
    // decode month
    dt.month = 1;
    for (size_t i = 0; i < 12; i++) {
        int month = MONTH_DAYS_ARRAY[2 * i + is_leap];
        if (ts_sec >= month) {
            ts_sec -= month;
        } else {
            dt.month += i;
            break;
        }
    }
    // decode left
    dt.day = 1;
    dt.day += ts_sec / DAY;
    ts_sec %= DAY;
    dt.hour = ts_sec / HOUR;
    ts_sec %= HOUR;
    dt.mintue = ts_sec / MINUTE;
    dt.second = ts_sec % MINUTE;
    return dt;
}

datetime get_datetime(int64_t ts_sec, int ts_nano, int zone = 0) {
    ts_sec += zone * time_const::HOUR;
    datetime dt{.zone = zone, .nano = ts_nano};
    if (ts_sec >= 0) {
        get_datetime_after(dt, ts_sec);
    } else {
        get_datetime_before(dt, -ts_sec);
    }
    return dt;
}

} // namespace log2what
#endif
