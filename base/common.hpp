/**
 * @file common.hpp
 * @author TNumFive
 * @brief Common utilities and structs for whole project.
 * @version 0.1
 * @date 2023-01-08
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef LOG2WHAT_COMMON_HPP
#define LOG2WHAT_COMMON_HPP

#include <chrono>
#include <string>

namespace log2what
{
    /**
     * @brief Enum class for log level, each level occupies one bit.
     */
    enum class log_level : int
    {
        TRACE = 1,
        DEBUG = 2,
        INFO = 4,
        WARN = 8,
        ERROR = 16
    };

    /**
     * @brief Convert log_level enum to string.
     * @details Use one single letter to represent the log_level. When unknown
     * level is given, the function returns "U", which stands for "Unknwon".
     * @param level Log level to be converted.
     * @return std::string Result string.
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

    /**
     * @brief Struct of log.
     */
    struct log
    {
        using string = std::string;
        int64_t timestamp_nano;
        log_level level;
        string module;
        string comment;
        string data;
        /**
         * @brief Construct a new log object.
         *
         * @param timestamp_nano Timestamp in nanoseconds.
         * @param level Log level.
         * @param module Name of module.
         * @param comment Content of log.
         * @param data Data attached.
         */
        log(const int64_t timestamp_nano, const log_level level,
            const string &module, const string &comment, const string &data)
        {
            this->timestamp_nano = timestamp_nano;
            this->level = level;
            this->module = module;
            this->comment = comment;
            this->data = data;
        }
        /**
         * @brief Construct a new log object with rvalue.
         *
         * @param timestamp_nano Timestamp in nanoseconds.
         * @param level Log level of log.
         * @param module Name of module.
         * @param comment Content of log.
         * @param data Data attached.
         */
        log(const int64_t timestamp_nano, const log_level level,
            string &&module, string &&comment, string &&data)
        {
            this->timestamp_nano = timestamp_nano;
            this->level = level;
            this->module = std::move(module);
            this->comment = std::move(comment);
            this->data = std::move(data);
        }
        /**
         * @brief Copy constructor.
         *
         * @param other Ohter log.
         */
        log(const log &other) { this->copy(other); };
        /**
         * @brief Copy assign constructor.
         *
         * @param other Other log.
         * @return log& Self.
         */
        log &operator=(const log &other)
        {
            this->copy(other);
            return *this;
        }
        /**
         * @brief Move constructor.
         *
         * @param other Other log.
         */
        log(log &&other) { this->swap(std::move(other)); };
        /**
         * @brief Move assign constructor.
         *
         * @param other Other log.
         * @return log& Self.
         */
        log &operator=(log &&other)
        {
            this->swap(std::move(other));
            return *this;
        }
        /**
         * @brief Implementation of copy action.
         *
         * @param other Other log.
         */
        void copy(const log &other)
        {
            this->timestamp_nano = other.timestamp_nano;
            this->level = other.level;
            this->module = other.module;
            this->comment = other.comment;
            this->data = other.data;
        }
        /**
         * @brief Implementation of move action.
         *
         * @param other Other log.
         */
        void swap(log &&other)
        {
            std::swap(this->timestamp_nano, other.timestamp_nano);
            std::swap(this->level, other.level);
            std::swap(this->module, other.module);
            std::swap(this->comment, other.comment);
            std::swap(this->data, other.data);
        }
    };

    /**
     * @brief Get timestamp.
     *
     * @tparam T Time precision from std::chrono.
     * @return int64_t Current timestamp in given precision.
     */
    template <typename T> inline int64_t get_timestamp()
    {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        return std::chrono::duration_cast<T>(now).count();
    }

    /**
     * @brief Get the timestamp in nanoseconds.
     *
     * @return int64_t Current timestamp in nanoseconds.
     */
    inline int64_t get_nano_timestamp()
    {
        return get_timestamp<std::chrono::nanoseconds>();
    }

    /**
     * @brief Get the localtime tm object.
     *
     * @param timestamp_sec Timestamp in seconds.
     * @return std::tm Broken-down time of given timestamp
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
     * @brief Get the localtime str.
     *
     * @param timestamp_sec Timestamp in seconds.
     * @return std::string Time string of given timestamp.
     */
    inline std::string get_localtime_str(const int64_t timestamp_sec)
    {
        char buffer[sizeof("2022-07-30 11:01:52")];
        std::tm lt = get_localtime_tm(timestamp_sec);
        std::strftime(buffer, sizeof(buffer), "%F %T", &lt);
        return buffer;
    }

    /**
     * @brief Make directory by cmd or bash script.
     *
     * @param dir_path Directory to create.
     */
    inline void mkdir(const std::string &dir_path)
    {
        using std::string;
#ifdef __WIN32__
        string cmd = "if not exist \"${dir}\" (md \"${dir}\")";
#else
        string cmd = "if [ ! -d \"${dir}\" ]; then \n\tmkdir -p \"${dir}\"\nfi";
#endif
        cmd.replace(cmd.find("${dir}"), 6, dir_path);
        cmd.replace(cmd.find("${dir}"), 6, dir_path);
        system(cmd.c_str());
    }
} // namespace log2what
#endif
