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
inline string to_string(level l) {
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
inline void to_string(string &s, level l) {
    s = to_string(l);
}

/**
 * @brief Get the timestamp object
 *
 * @tparam T type for duration_cast
 * @return int64_t
 */
template <typename T>
inline int64_t get_timestamp() {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<T>(now).count();
}

/**
 * @brief Get the nano timestamp object
 *
 * @return int64_t
 */
inline int64_t get_nano_timestamp() {
    return get_timestamp<std::chrono::nanoseconds>();
}

/**
 * @brief Get the localtime tm object
 *
 * @param timestamp_sec
 * @return std::tm
 */
inline std::tm get_localtime_tm(int64_t timestamp_sec) {
#if defined __USE_POSIX || __GLIBC_USE(ISOC2X)
    std::tm lt;
    localtime_r(&timestamp_sec, &lt);
#elif
    std::tm lt = *std::localtime(&timestamp_sec);
#endif
    return lt;
}

/**
 * @brief Get the localtime str of specific unix-stamp
 *
 * @param timestamp_sec
 * @return string like 2022-07-30 11:01:52
 */
inline string get_localtime_str(int64_t timestamp_sec) {
    char buffer[20];
    std::tm lt = get_localtime_tm(timestamp_sec);
    std::strftime(buffer, sizeof(buffer), "%F %T", &lt);
    return buffer;
}

/**
 * @brief make dir with cmd
 *
 * @param dir_path
 */
inline void mkdir(string dir_path) {
#ifdef __WIN32__
    string cmd = "if not exist \"${dir}\" (md \"${dir}\")";
    dir_path.replace(dir_path.begin(), dir_path.end(), '/', '\\');
#else
    string cmd = "if [ ! -d \"${dir}\" ]; then \n\tmkdir -p \"${dir}\"\nfi";
#endif
    cmd.replace(cmd.find("${dir}"), 6, dir_path);
    cmd.replace(cmd.find("${dir}"), 6, dir_path);
    system(cmd.c_str());
}
} // namespace log2what
#endif
