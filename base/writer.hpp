#ifndef WRITER_HPP
#define WRITER_HPP

#include "./common.hpp"
#include <iomanip>
#include <iostream>
#include <string>

namespace log2what
{
    /**
     * @brief base writer class
     *
     */
    class writer
    {
    private:
        using string = std::string;

    public:
        writer() = default;
        writer(const writer &other) = delete;
        writer(writer &&other) = delete;
        writer &operator=(const writer &other) = delete;
        writer &operator=(writer &&other) = delete;
        virtual ~writer() = default;
        virtual void write(const log_level level, const string &module_name,
                           const string &comment, const string &data,
                           const int64_t timestamp_nano = 0)
        {
            constexpr int sec_to_nano = 1000000000;
            constexpr int milli_to_nano = 1000000;
            int64_t nano = timestamp_nano ? timestamp_nano : get_nano_timestamp();
            int64_t sec = nano / sec_to_nano;
            int64_t precision = (nano % sec_to_nano) / milli_to_nano;
            std::cout << get_localtime_str(sec)
                      << "." << std::setw(3) << std::setfill('0') << precision
                      << " " << to_string(level)
                      << " " << module_name
                      << " |%| " << comment
                      << " |%| " << data << std::endl;
        }
    };
} // namespace log2what
#endif