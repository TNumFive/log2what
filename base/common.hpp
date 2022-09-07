#ifndef COMMON_HPP
#define COMMON_HPP

#include <chrono>
#include <string>

namespace log2what
{
    enum class log_level : int
    {
        TRACE = 1,
        DEBUG = 2,
        INFO = 4,
        WARN = 8,
        ERROR = 16
    };

    /**
     * @brief convert log_level enum to string
     *
     * @param level enum level
     * @return string
     */
    inline std::string to_string(const log_level level)
    {
        switch (level)
        {
        case log_level::TRACE:
            return "T";
        case log_level::DEBUG:
            return "D";
        case log_level::INFO:
            return "I";
        case log_level::WARN:
            return "W";
        case log_level::ERROR:
            return "E";
        default:
            return "U";
        }
    }

    struct log
    {
        using string = std::string;
        int64_t timestamp_nano;
        log_level level;
        string module_name;
        string comment;
        string data;
        log(const int64_t timestamp_nano, const log_level level,
            const string &module_name, const string &comment, const string &data)
        {
            this->timestamp_nano = timestamp_nano;
            this->level = level;
            this->module_name = module_name;
            this->comment = comment;
            this->data = data;
        }
        log(const int64_t timestamp_nano, const log_level level,
            string &&module_name, string &&comment, string &&data)
        {
            this->timestamp_nano = timestamp_nano;
            this->level = level;
            this->module_name = std::move(module_name);
            this->comment = std::move(comment);
            this->data = std::move(data);
        }
    };

    /**
     * @brief Get the timestamp object
     *
     * @tparam T type for duration_cast
     * @return int64_t
     */
    template <typename T>
    inline int64_t get_timestamp()
    {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        return std::chrono::duration_cast<T>(now).count();
    }

    /**
     * @brief Get the nano timestamp object
     *
     * @return int64_t
     */
    inline int64_t get_nano_timestamp()
    {
        return get_timestamp<std::chrono::nanoseconds>();
    }

    /**
     * @brief Get the localtime tm object
     *
     * @param timestamp_sec
     * @return std::tm
     */
    inline std::tm get_localtime_tm(const int64_t timestamp_sec)
    {
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
    inline std::string get_localtime_str(const int64_t timestamp_sec)
    {
        char buffer[sizeof("2022-07-30 11:01:52")];
        std::tm lt = get_localtime_tm(timestamp_sec);
        std::strftime(buffer, sizeof(buffer), "%F %T", &lt);
        return buffer;
    }

    /**
     * @brief make dir with cmd
     *
     * @param dir_path
     */
    inline void mkdir(const std::string &dir_path)
    {
#ifdef __WIN32__
        std::string cmd = "if not exist \"${dir}\" (md \"${dir}\")";
#else
        std::string cmd = "if [ ! -d \"${dir}\" ]; then \n\tmkdir -p \"${dir}\"\nfi";
#endif
        cmd.replace(cmd.find("${dir}"), 6, dir_path);
        cmd.replace(cmd.find("${dir}"), 6, dir_path);
        system(cmd.c_str());
    }
} // namespace log2what
#endif
