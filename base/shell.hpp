/**
 * @file shell.hpp
 * @author TNumFive
 * @brief Example of shell class
 * @version 0.1
 * @date 2023-01-11
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef LOG2WHAT_SHELL_HPP
#define LOG2WHAT_SHELL_HPP

#include "./common.hpp"
#include "./writer.hpp"
#include <memory>

namespace log2what
{
    /**
     * @brief Base shell with function of masking log by log levels.
     */
    class shell : public writer
    {
    private:
        using string = std::string;
        using unique_ptr_writer = std::unique_ptr<writer>;
        unique_ptr_writer writer_unique_ptr;
        log_level mask;

    public:
        /**
         * @brief Construct a new shell object.
         *
         * @param mask Least log level that won't be masked.
         * @param writer_unique_ptr Writer.
         */
        shell(const log_level mask = log_level::INFO,
              unique_ptr_writer &&writer_unique_ptr = unique_ptr_writer{
                  new writer})
        {
            this->mask = mask;
            this->writer_unique_ptr = std::move(writer_unique_ptr);
        }
        /**
         * @brief Copy constructor deleted.
         *
         * @param other Other shell.
         */
        shell(const shell &other) = delete;
        /**
         * @brief Copy assign constructor deleted.
         *
         * @param other Other shell.
         * @return shell& Self.
         */
        shell &operator=(const shell &other) = delete;
        /**
         * @brief Move constructor deleted.
         *
         * @param other Other shell.
         */
        shell(shell &&other) = delete;
        /**
         * @brief Move assign constructor.
         *
         * @param other Other shell.
         * @return shell& Self.
         */
        shell &operator=(shell &&other) = delete;
        /**
         * @brief Defaut Destructor.
         *
         */
        ~shell() override = default;
        /**
         * @brief Write log.
         *
         * @param level Log level.
         * @param module Module name.
         * @param comment Content of log.
         * @param data Data attached.
         * @param timestamp_nano Timestamp of log in nanoseconds.
         */
        void write(const log_level level, const string &module,
                   const string &comment, const string &data,
                   const int64_t timestamp_nano) override
        {
            if (level >= this->mask)
            {
                this->writer_unique_ptr->write(level, module, comment, data);
            }
        }
    };
} // namespace log2what
#endif