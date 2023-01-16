/**
 * @file writer.hpp
 * @author TNumFive
 * @brief Base writer.
 * @version 0.1
 * @date 2023-01-13
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef LOG2WHAT_WRITER_HPP
#define LOG2WHAT_WRITER_HPP

#include "./common.hpp"
#include <iomanip>
#include <iostream>
#include <string>

namespace log2what
{
    /**
     * @brief Base writer class.
     */
    class writer
    {
    public:
        using string = std::string;
        /**
         * @brief Default constructor.
         */
        writer() = default;
        /**
         * @brief Copy constructor deleted.
         *
         * @param other Other writer.
         */
        writer(const writer &other) = delete;
        /**
         * @brief Copy assign constructor deleted.
         *
         * @param other Other writer.
         * @return writer& Self.
         */
        writer &operator=(const writer &other) = delete;
        /**
         * @brief Move constructor deleted.
         *
         * @param other Other writer.
         */
        writer(writer &&other) = delete;
        /**
         * @brief Move assign constructor deleted.
         *
         * @param other Other writer.
         * @return writer& Self.
         */
        writer &operator=(writer &&other) = delete;
        /**
         * @brief Default destructor.
         *
         */
        virtual ~writer() = default;
        /**
         * @brief Write log.
         *
         * @param level Log level.
         * @param module Module name.
         * @param comment Content of log.
         * @param data Data attached.
         * @param timestamp_nano Timestamp of log in nanoseconds.
         */
        virtual void write(const log_level level, const string &module,
                           const string &comment, const string &data,
                           const int64_t timestamp_nano = 0)
        {
            constexpr int sec_to_nano = 1000000000;
            constexpr int milli_to_nano = 1000000;
            int64_t nano =
                timestamp_nano ? timestamp_nano : get_nano_timestamp();
            int64_t sec = nano / sec_to_nano;
            int64_t precision = (nano % sec_to_nano) / milli_to_nano;
            std::cout << get_localtime_str(sec);
            std::cout << "." << std::setw(3) << std::setfill('0') << precision;
            std::cout << " " << to_string(level);
            std::cout << " " << module;
            std::cout << " |%| " << comment;
            std::cout << " |%| " << data << std::endl;
        }
    };
} // namespace log2what
#endif