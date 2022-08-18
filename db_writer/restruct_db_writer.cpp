#include "../base/common.hpp"
#include "../base/log2what.hpp"
#include "./db_writer.hpp"
#include <list>
#include <map>
#include <mutex>
#include <sqlite3.h>

using namespace std;
using namespace log2what;

struct log {
    int64_t timestamp_nano;
    level l;
    string module_name;
    string comment;
    string data;
    log(const int64_t timestamp_nano, const level l,
        const string &module_name, const string &comment, const string &data) {
        this->timestamp_nano = timestamp_nano;
        this->l = l;
        this->module_name = module_name;
        this->comment = comment;
        this->data = data;
    }
    log(const int64_t timestamp_nano, const level l,
        string &&module_name, string &&comment, string &&data) {
        this->timestamp_nano = timestamp_nano;
        this->l = l;
        this->module_name = std::move(module_name);
        this->comment = std::move(comment);
        this->data = std::move(data);
    }
};

struct db_info {
    mutex db_mutex;
    sqlite3 *db_ptr;
    sqlite3_stmt *batch_ptr;
    list<log> log_list;
    size_t writer_num;
    size_t buffer_size;
    log2 *logger_ptr;
    string url;
    int create_table() {
        constexpr char create_table[] =
            "create table if not exists log("
            "timestamp int primary key not null,"
            "level int,"
            "module_name text,"
            "comment text,"
            "data text"
            ");";
        return sqlite3_exec(db_ptr, create_table, nullptr, nullptr, nullptr);
    }
    int flush() {
        using leve_type = std::underlying_type_t<level>;
        size_t param_index = 0;
        int ret = SQLITE_OK;
        auto cursor = log_list.begin();
        for (size_t i = 0; i < buffer_size && cursor != log_list.end(); i++) {
            const auto &l = *cursor;
            do {
                ret = sqlite3_bind_int64(batch_ptr, ++param_index, l.timestamp_nano);
                if (ret != SQLITE_OK) {
                    logger_ptr->error("bind timestamp failed", sqlite3_errmsg(db_ptr));
                    break;
                }
                ret = sqlite3_bind_int64(batch_ptr, ++param_index, static_cast<leve_type>(l.l));
                if (ret != SQLITE_OK) {
                    logger_ptr->error("bind level failed", sqlite3_errmsg(db_ptr));
                    break;
                }
                ret = sqlite3_bind_text(batch_ptr, ++param_index, l.module_name.c_str(), l.module_name.size(), SQLITE_STATIC);
                if (ret != SQLITE_OK) {
                    logger_ptr->error("bind module_name failed", sqlite3_errmsg(db_ptr));
                    break;
                }
                ret = sqlite3_bind_text(batch_ptr, ++param_index, l.comment.c_str(), l.comment.size(), SQLITE_STATIC);
                if (ret != SQLITE_OK) {
                    logger_ptr->error("bind comment failed", sqlite3_errmsg(db_ptr));
                    break;
                }
                ret = sqlite3_bind_text(batch_ptr, ++param_index, l.data.c_str(), l.data.size(), SQLITE_STATIC);
                if (ret != SQLITE_OK) {
                    logger_ptr->error("bind data failed", sqlite3_errmsg(db_ptr));
                    break;
                }
            } while (false);
            if (ret != SQLITE_OK && ret != SQLITE_DONE) {
                break;
            } else {
                ret = SQLITE_OK;
            }
            cursor++;
        }
        if (!ret) {
            ret = sqlite3_step(batch_ptr);
            if (ret != SQLITE_DONE) {
                logger_ptr->error("step stmt failed", sqlite3_errmsg(db_ptr));
            } else {
                ret = SQLITE_OK;
            }
        }
        ret = sqlite3_reset(batch_ptr);
        if (ret != SQLITE_OK) {
            logger_ptr->error("reset stmt failed", sqlite3_errmsg(db_ptr));
        }
        std::stringstream ss;
        for (size_t i = 0; i < buffer_size; i++) {
            auto &l = log_list.front();
            if (ret) {
                ss.str("");
                ss << "("
                   << l.timestamp_nano << ","
                   << to_string(l.l) << ","
                   << l.comment << ","
                   << l.data
                   << ")";
                logger_ptr->error("insert into db failed", ss.str());
            }
            log_list.pop_front();
        }
        return ret;
    }
    int reload_batch_stmt(const size_t buffer_szie) {
        constexpr char insert[] = "insert into log values";
        constexpr char values[] = "(?,?,?,?,?),";
        constexpr char last_value[] = "(?,?,?,?,?);";
        int ret = SQLITE_OK;
        if (this->buffer_size != buffer_szie) {
            if (batch_ptr != nullptr) {
                ret = sqlite3_finalize(batch_ptr);
                if (ret != SQLITE_OK) {
                    logger_ptr->error("buffer_size changed, finalize stmt failed", sqlite3_errmsg(db_ptr));
                    batch_ptr = nullptr;
                    return ret;
                }
            }
            this->buffer_size = buffer_szie;
            string sql{insert};
            for (size_t i = 0; i < buffer_size - 1; i++) {
                sql.append(values);
            }
            sql.append(last_value);
            ret = sqlite3_prepare_v2(db_ptr, sql.c_str(), sql.size(), &batch_ptr, nullptr);
            if (ret != SQLITE_OK) {
                logger_ptr->error("buffer_size changed, prepare stmt failed", sqlite3_errmsg(db_ptr));
                batch_ptr = nullptr;
                return ret;
            }
        }
        return ret;
    }
    ~db_info() {
        if (!log_list.empty()) {
            reload_batch_stmt(log_list.size());
            flush();
        }
        if (batch_ptr != nullptr) {
            if (SQLITE_OK != sqlite3_finalize(batch_ptr)) {
                logger_ptr->error("destruct db_info, finalize stmt failed", sqlite3_errmsg(db_ptr));
            }
            batch_ptr = nullptr;
        }
        if (db_ptr != nullptr) {
            if (SQLITE_OK != sqlite3_close(db_ptr)) {
                logger_ptr->error("destruct db_info, close db failed", sqlite3_errmsg(db_ptr));
            }
            db_ptr = nullptr;
        }
        if (logger_ptr != nullptr) {
            delete logger_ptr;
            logger_ptr = nullptr;
        }
    }
};

static map<string, db_info> db_info_map;
static mutex life_cycle_mutex;

void db_writer::flush_log_list() {}

db_writer::db_writer(const string &url, const size_t buffer_szie, writer *writer_ptr) {
    lock_guard<mutex> life_cycle_lock{life_cycle_mutex};
    auto &info = db_info_map[url];
    db_info_ptr = static_cast<void *>(&info);
    info.writer_num++;
    if (info.db_ptr == nullptr) {
        size_t deli = url.find_last_of('/');
        mkdir(url.substr(0, deli));
        info.url = url;
        info.logger_ptr = new log2{"log2db", writer_ptr};
        if (SQLITE_OK != sqlite3_open(url.c_str(), &info.db_ptr)) {
            info.logger_ptr->error("open db failed", sqlite3_errmsg(info.db_ptr));
            return;
        }
        if (SQLITE_OK != info.create_table()) {
            info.logger_ptr->error("create_table failed", sqlite3_errmsg(info.db_ptr));
            return;
        }
    }
    lock_guard<mutex> db_lock{info.db_mutex};
    info.reload_batch_stmt(buffer_szie);
}

db_writer::~db_writer() {
    auto &info = *(static_cast<db_info *>(db_info_ptr));
    lock_guard<mutex> life_cycle_lock{life_cycle_mutex};
    info.writer_num--;
}

void db_writer::write(const level l, const string &module_name, const string &comment, const string &data) {
    auto &info = *(static_cast<db_info *>(db_info_ptr));
    lock_guard<mutex> db_lock{info.db_mutex};
    info.log_list.emplace_back(get_nano_timestamp(), l, module_name, comment, data);
    while (info.log_list.size() >= info.buffer_size) {
        info.flush();
    }
}