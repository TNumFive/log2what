/**
 * @file db_writer.cpp
 * @author TNumFive
 * @brief Implementation of example db_writer.
 * @version 0.1
 * @date 2023-01-11
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "./db_writer.hpp"
#include "../base/common.hpp"
#include "../base/log2what.hpp"
#include <dirent.h>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <sqlite3.h>

using namespace std;
using namespace log2what;

/**
 * @brief Every log has 5 columns.
 *
 */
static constexpr size_t log_column = 5;
/**
 * @brief The max number of variables to be bind.
 *
 * The number is referred by https://sqlite.org/c3ref/bind_blob.html
 *
 */
static constexpr size_t limit_variable_number = 32766;
/**
 * @brief The max number of logs to be buffered.
 *
 */
static constexpr size_t max_buffer_size = limit_variable_number / log_column;

/**
 * @brief Helper class of sqlite3
 *
 */
class sqlite3_helper
{
public:
    /**
     * @brief Construct a new sqlite3 helper object.
     *
     * @param file_path Path of database.
     * @param buffer_size Buffer size of log's buffer.
     * @param logger_unique_ptr Logger used to write database's logs.
     */
    sqlite3_helper(const string file_path, const size_t buffer_size,
                   unique_ptr<log2one> &&logger_unique_ptr =
                       unique_ptr<log2one>(new log2one))
    {
        this->logger_unique_ptr = std::move(logger_unique_ptr);
        this->file_path = file_path;
        size_t delimiter = file_path.find_last_of('/');
        string file_dir = file_path.substr(0, delimiter);
        auto dir_ptr = opendir(file_dir.c_str());
        if (dir_ptr == nullptr)
        {
            mkdir(file_path.substr(0, delimiter));
        }
        else
        {
            closedir(dir_ptr);
        }
        if (SQLITE_OK != sqlite3_open(file_path.c_str(), &this->db_ptr))
        {
            this->logger_unique_ptr->error("open db failed",
                                           sqlite3_errmsg(this->db_ptr));
            return;
        }
        if (SQLITE_OK != this->create_table())
        {
            return;
        }
        this->reload_stmt_ptr(buffer_size <= max_buffer_size ? buffer_size
                                                             : max_buffer_size);
    }
    /**
     * @brief Copy constructor deleted.
     *
     * @param other Other helpre.
     */
    sqlite3_helper(const sqlite3_helper &other) = delete;
    /**
     * @brief Copy assign constructor deleted.
     *
     * @param other Other helpre.
     * @return sqlite3_helper& Self.
     */
    sqlite3_helper &operator=(const sqlite3_helper &other) = delete;
    /**
     * @brief Move constructor deleted.
     *
     * @param other Other helpre.
     */
    sqlite3_helper(sqlite3_helper &&other) = delete;
    /**
     * @brief Move assign constructor deleted.
     *
     * @param other Other helpre.
     * @return sqlite3_helper& Self.
     */
    sqlite3_helper &operator=(sqlite3_helper &&other) = delete;
    /**
     * @brief Destroy the sqlite3 helper object.
     *
     */
    ~sqlite3_helper()
    {
        if (this->log_list.size())
        {
            this->flush_if_full();
            if (this->log_list.size())
            {
                this->reload_stmt_ptr(this->log_list.size());
                this->flush();
            }
        }
        if (this->stmt_ptr != nullptr)
        {
            if (SQLITE_OK != sqlite3_finalize(this->stmt_ptr))
            {
                this->logger_unique_ptr->error(
                    "destructing, finalize stmt failed",
                    sqlite3_errmsg(this->db_ptr));
            }
            this->stmt_ptr = nullptr;
        }
        if (this->db_ptr != nullptr)
        {
            if (SQLITE_OK != sqlite3_close(this->db_ptr))
            {
                this->logger_unique_ptr->error("destructing, close db failed",
                                               sqlite3_errmsg(this->db_ptr));
            }
            this->db_ptr = nullptr;
        }
    }
    /**
     * @brief Buffer and write logs to database.
     *
     * @param level Log level.
     * @param module Module name.
     * @param comment Content of log.
     * @param data Data attached.
     * @param timestamp_nano Timestamp of log in nano.
     */
    void write(const log_level level, const string &module,
               const string &comment, const string &data,
               const int64_t timestamp_nano)
    {
        lock_guard<mutex> db_lock{this->db_mutex};
        this->log_list.emplace_back(timestamp_nano, level, module, comment,
                                    data);
        this->flush_if_full();
    }

private:
    string file_path;
    size_t buffer_size;
    sqlite3 *db_ptr;
    sqlite3_stmt *stmt_ptr;
    list<log> log_list;
    unique_ptr<log2one> logger_unique_ptr;
    mutex db_mutex;

    /**
     * @brief Create table for logs when first open database.
     *
     * @return int Return SQLITE_OK if no error happened.
     */
    int create_table()
    {
        constexpr char create_table[] = "create table if not exists log("
                                        "timestamp int primary key not null,"
                                        "level int,"
                                        "module text,"
                                        "comment text,"
                                        "data text"
                                        ");";
        int ret =
            sqlite3_exec(this->db_ptr, create_table, nullptr, nullptr, nullptr);
        if (ret != SQLITE_OK)
        {
            this->logger_unique_ptr->error("create_table failed",
                                           sqlite3_errmsg(this->db_ptr));
        }
        return ret;
    }
    /**
     * @brief Prepare statement of inserting logs.
     *
     * @param buffer_size Write how many logs at once.
     * @return int Return SQLITE_OK if no error happened.
     */
    int prepare_stmt(const size_t buffer_size)
    {
        constexpr char insert[] = "insert into log values";
        constexpr char values[] = "(?,?,?,?,?),";
        constexpr char last_value[] = "(?,?,?,?,?);";
        int ret = SQLITE_OK;
        string sql{insert};
        for (size_t i = 0; i < buffer_size - 1; i++)
        {
            sql.append(values);
        }
        if (buffer_size)
        {
            sql.append(last_value);
        }
        ret = sqlite3_prepare_v2(this->db_ptr, sql.c_str(), sql.size(),
                                 &this->stmt_ptr, nullptr);
        if (ret != SQLITE_OK)
        {
            this->logger_unique_ptr->error("prepare stmt_ptr failed",
                                           sqlite3_errmsg(this->db_ptr));
        }
        return ret;
    }
    /**
     * @brief Release the old statement and prepare a new one.
     *
     * @param buffer_size Write how many logs at once.
     * @return int Return SQLITE_OK if no error happened.
     */
    int reload_stmt_ptr(const size_t buffer_size)
    {
        int ret = SQLITE_OK;
        this->buffer_size = buffer_size;
        if (this->stmt_ptr != nullptr)
        {
            ret = sqlite3_finalize(this->stmt_ptr);
            if (ret != SQLITE_OK)
            {
                this->logger_unique_ptr->error(
                    "finalize stmt_ptr failed when reload stmt_ptr",
                    sqlite3_errmsg(this->db_ptr));
                return ret;
            }
        }
        ret = this->prepare_stmt(buffer_size);
        return ret;
    }
    /**
     * @brief Flush buffered logs to database.
     * @details Please make sure that enough logs have been buffered.
     * @return int Return SQLITE_OK if no error happened.
     */
    int flush()
    {
        using level_type = std::underlying_type_t<log_level>;
        int ret = SQLITE_OK;
        size_t param_index = 0;
        list<log> temp_log_list;
        for (size_t i = 0; i < this->buffer_size; i++)
        {
            temp_log_list.push_back(std::move(this->log_list.front()));
            this->log_list.pop_front();
            auto &temp_log = temp_log_list.back();
            ret = sqlite3_bind_int64(this->stmt_ptr, ++param_index,
                                     temp_log.timestamp_nano);
            if (ret != SQLITE_OK)
            {
                this->logger_unique_ptr->error("bind timestamp failed",
                                               sqlite3_errmsg(this->db_ptr));
                break;
            }
            ret = sqlite3_bind_int64(this->stmt_ptr, ++param_index,
                                     static_cast<level_type>(temp_log.level));
            if (ret != SQLITE_OK)
            {
                this->logger_unique_ptr->error("bind level failed",
                                               sqlite3_errmsg(this->db_ptr));
                break;
            }
            ret = sqlite3_bind_text(this->stmt_ptr, ++param_index,
                                    temp_log.module.c_str(),
                                    temp_log.module.size(), SQLITE_STATIC);
            if (ret != SQLITE_OK)
            {
                this->logger_unique_ptr->error("bind module failed",
                                               sqlite3_errmsg(db_ptr));
                break;
            }
            ret = sqlite3_bind_text(this->stmt_ptr, ++param_index,
                                    temp_log.comment.c_str(),
                                    temp_log.comment.size(), SQLITE_STATIC);
            if (ret != SQLITE_OK)
            {
                this->logger_unique_ptr->error("bind comment failed",
                                               sqlite3_errmsg(db_ptr));
                break;
            }
            ret = sqlite3_bind_text(this->stmt_ptr, ++param_index,
                                    temp_log.data.c_str(), temp_log.data.size(),
                                    SQLITE_STATIC);
            if (ret != SQLITE_OK)
            {
                this->logger_unique_ptr->error("bind data failed",
                                               sqlite3_errmsg(db_ptr));
                break;
            }
        }
        if (ret == SQLITE_OK)
        {
            ret = sqlite3_step(this->stmt_ptr);
            if (ret != SQLITE_DONE)
            {
                this->logger_unique_ptr->error("step stmt failed",
                                               sqlite3_errmsg(db_ptr));
            }
        }
        else
        {
            this->logger_unique_ptr->error("error happened when try step stmt");
            for (auto &temp_log : temp_log_list)
            {
                stringstream ss;
                ss << "{\"comment\":\"" << temp_log.comment << "\", "
                   << "\"data\":\"" << temp_log.data << "\"}";
                this->logger_unique_ptr->write(
                    temp_log.level, to_string(temp_log.timestamp_nano),
                    ss.str());
            }
        }
        ret = sqlite3_reset(this->stmt_ptr);
        if (ret != SQLITE_OK)
        {
            this->logger_unique_ptr->error("reset stmt failed",
                                           sqlite3_errmsg(db_ptr));
        }
        return ret;
    }
    /**
     * @brief Flush logs to database if enough logs have been buffered.
     *
     */
    void flush_if_full()
    {
        while (this->log_list.size() >= this->buffer_size)
        {
            this->flush();
        }
    }
};

static mutex life_cycle_mutex;
/**
 * @brief Center map that stores all helper of opened sqlite3 database.
 *
 */
static map<string, unique_ptr<sqlite3_helper>> helper_map;

db_writer::db_writer(const string &file_path, const size_t buffer_szie,
                     unique_ptr_writer &&writer_unique_ptr)
{
    lock_guard<mutex> life_cycle_lock{::life_cycle_mutex};
    this->file_path = file_path;
    if (helper_map.count(file_path))
    {
        return;
    }
    auto logger_unique_ptr = unique_ptr<log2one>{
        new log2one{"db_writer", std::move(writer_unique_ptr)}};
    helper_map[file_path].reset(new sqlite3_helper{
        file_path, buffer_szie, std::move(logger_unique_ptr)});
}

void db_writer::write(const log_level level, const string &module,
                      const string &comment, const string &data,
                      const int64_t timestamp_nano)
{
    unique_lock<mutex> life_cycle_lock{::life_cycle_mutex};
    auto &helper = helper_map[this->file_path];
    life_cycle_lock.unlock();
    int64_t timestamp = timestamp_nano ? timestamp_nano : get_nano_timestamp();
    helper->write(level, module, comment, data, timestamp);
}