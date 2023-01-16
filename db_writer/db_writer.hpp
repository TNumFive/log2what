/**
 * @file db_writer.hpp
 * @author TNumFive
 * @brief Example of writer that writes to database.
 * @version 0.1
 * @date 2023-01-11
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef LOG2WHAT_DB_WRITER_HPP
#define LOG2WHAT_DB_WRITER_HPP

#include "../base/writer.hpp"

namespace log2what
{
    /**
     * @brief Writer that writes log to database(sqlite3).
     */
    class db_writer : public writer
    {
    public:
        using string = std::string;
        using unique_ptr_writer = std::unique_ptr<writer>;
        /**
         * @brief Construct a new db writer object.
         *
         * @param file_path File path of sqlite3 database file.
         * @param buffer_szie Buffer how many logs before writing to database.
         * @param writer_uptr Writer for db_writer's own logs.
         */
        db_writer(const string &file_path = "./log/log2.db",
                  const size_t buffer_szie = 100,
                  unique_ptr_writer &&writer_uptr = unique_ptr_writer{
                      new writer});
        /**
         * @brief Copy constructor deleted.
         *
         * @param other Other db_writer.
         */
        db_writer(const db_writer &other) = delete;
        /**
         * @brief Copy assign constructor deleted.
         *
         * @param other Other db_writer.
         * @return db_writer& Self.
         */
        db_writer &operator=(const db_writer &other) = delete;
        /**
         * @brief Move constructor deleted.
         *
         * @param other Other db_writer.
         */
        db_writer(db_writer &&other) = delete;
        /**
         * @brief Move assign constructor deleted.
         *
         * @param other Other db_writer.
         * @return db_writer& Self.
         */
        db_writer &operator=(db_writer &&other) = delete;
        /**
         * @brief Destroy the db writer object
         */
        ~db_writer() override{};
        /**
         * @brief Buffer logs and write all buffered at once.
         *
         * @param level Log level.
         * @param module Module name.
         * @param comment Content of log.
         * @param data Data attached.
         * @param timestamp_nano Timestamp of log in nano seconds.
         */
        void write(const log_level level, const string &module,
                   const string &comment, const string &data,
                   const int64_t timestamp_nano) override;

    private:
        string file_path;
    };
} // namespace log2what

#endif