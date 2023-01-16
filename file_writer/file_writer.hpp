/**
 * @file file_writer.hpp
 * @author TNumFive
 * @brief Example of writer that writes to file
 * @version 0.1
 * @date 2023-01-12
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef LOG2WHAT_FILE_WRITER_HPP
#define LOG2WHAT_FILE_WRITER_HPP

#include "../base/writer.hpp"

namespace log2what
{
    static constexpr size_t KB = 1024;
    static constexpr size_t MB = 1024 * KB;

    /**
     * @brief Writer that writes to file.
     */
    class file_writer : public writer
    {
    public:
        using string = std::string;
        /**
         * @brief Construct a new file writer object
         *
         * @param file_name File name of log file without extension.
         * @param file_dir Directory that stores log file.
         * @param file_size The max size of log file in bytes.
         * @param file_num The max file nums of log file rotation.
         * @param keep_alive Shall file be closed when no writer writes.
         */
        file_writer(const string &file_name = "root",
                    const string &file_dir = "./log/",
                    const size_t file_size = MB, const size_t file_num = 50);
        /**
         * @brief Copy constructor deleted.
         *
         * @param other Other writer.
         */
        file_writer(const file_writer &other) = delete;
        /**
         * @brief Copy asign constructor deleted.
         *
         * @param other Other writer.
         * @return file_writer& Self.
         */
        file_writer &operator=(const file_writer &other) = delete;
        /**
         * @brief Move constructor deleted.
         *
         * @param other Other writer.
         */
        file_writer(file_writer &&other) = delete;
        /**
         * @brief Move assign constructor deleted.
         *
         * @param other Other writer.
         * @return file_writer& Self.
         */
        file_writer &operator=(file_writer &&other) = delete;
        /**
         * @brief Destroy the file writer object
         *
         */
        ~file_writer() override = default;
        /**
         * @brief Write logs to file.
         *
         * @param level Log level.
         * @param module Module name.
         * @param comment Content of log.
         * @param data Data attached.
         * @param timestamp_nano Timestamp of log in nanoseconds.
         */
        void write(const log_level level, const string &module,
                   const string &comment, const string &data,
                   const int64_t timestamp_nano = 0) override;

    private:
        string helper_map_key;
    };
} // namespace log2what

#endif