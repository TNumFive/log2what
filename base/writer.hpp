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
    public:
        writer() = default;
        writer(const writer &other) = delete;
        writer(writer &&other) = delete;
        writer &operator=(const writer &other) = delete;
        writer &operator=(writer &&other) = delete;
        virtual ~writer() = default;
        virtual void write(const level l, const string &module_name, const string &comment, const string &data)
        {
            constexpr int SEC_TO_NANO = 1000000000;
            constexpr int MILL_TO_NANO = 1000000;
            int64_t timestamp_nano = get_nano_timestamp();
            int64_t timestamp_sec = timestamp_nano / SEC_TO_NANO;
            int64_t precision = (timestamp_nano % SEC_TO_NANO) / MILL_TO_NANO;
            std::cout << get_localtime_str(timestamp_sec)
                      << "." << std::setw(3) << std::setfill('0') << precision
                      << " " << to_string(l)
                      << " " << module_name
                      << " |%| " << comment
                      << " |%| " << data << std::endl;
        }
    };
} // namespace log2what
#endif