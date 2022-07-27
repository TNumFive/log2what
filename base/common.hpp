#ifndef COMMON_HPP
#define COMMON_HPP

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

namespace log2what {
using std::string;

/**
 * @brief level for log
 *
 */
enum class level : int {
    TRACE = 1,
    DEBUG = 2,
    INFO = 4,
    WARN = 8,
    ERROR = 16
};

/**
 * @brief convert log level enum to string
 *
 * @param l enum level
 * @return string
 */
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

/**
 * @brief convert log level to string
 *
 * @param s result string
 * @param l input level
 */
void to_string(string &s, level l) {
    s = to_string(l);
}

/**
 * @brief Get the timestamp object
 *
 * @tparam T type for duration_cast
 * @return int64_t
 */
template <typename T>
int64_t get_timestamp() {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<T>(now).count();
}

/**
 * @brief Get the nano timestamp object
 *
 * @return int64_t
 */
int64_t get_nano_timestamp() {
    return get_timestamp<std::chrono::nanoseconds>();
}

/**
 * @brief time lock as for time-related is not thread-safe at c++11
 *
 */
static std::mutex time_lock;

/**
 * @brief datetime object with struct tm of time.h wrapped
 * @note internal tm object is already adjusted by time zone, so don't use str-fn in ctime
 */
struct datetime {
    std::tm tp;
    int zone;
    int nano;
    string to_string() const {
        using namespace std;
        stringstream ss;
        ss << tp.tm_year + 1900
           << "-" << setfill('0') << setw(2) << tp.tm_mon + 1
           << "-" << setfill('0') << setw(2) << tp.tm_mday
           << "T" << setfill('0') << setw(2) << tp.tm_hour
           << ":" << setfill('0') << setw(2) << tp.tm_min
           << ":" << fixed << setfill('0') << setw(6)
           << setprecision(3) << nano / 1e9 + tp.tm_sec;
        if (zone) {
            ss << setiosflags(ios::showpos) << zone;
        } else {
            ss << "Z";
        }
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

/**
 * @brief Get the datetime object
 *
 * @param ts_sec timestamp in seconds
 * @param ts_nano nano part of timestamp for better percision
 * @param zone time zone
 * @return datetime
 */
datetime get_datetime(int64_t ts_sec, int ts_nano, int zone = 0) {
    constexpr int HOUR_TO_SECOND = 3600;
    ts_sec += zone * HOUR_TO_SECOND;
    datetime dt{.zone = zone, .nano = ts_nano};
    std::lock_guard<std::mutex> lock{time_lock};
    dt.tp = *std::gmtime(&ts_sec);
    return dt;
}
} // namespace log2what
#endif
