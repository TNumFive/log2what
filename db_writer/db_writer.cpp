#include "./db_writer.hpp"
#include "../base/common.hpp"
#include "../base/log2what.hpp"
#include <list>
#include <map>
#include <mutex>
#include <sqlite3.h>
#include <thread>

using namespace log2what;

struct log {
    int64_t timestamp_nano;
    level l;
    string moudle_name;
    string comment;
    string data;
};

struct db_info {
    string url;
    size_t writer_num;
    sqlite3 *db_ptr;
    sqlite3_stmt *stmt_ptr;
    std::list<log> log_list;
    size_t buffer_size;
    log2 *logger_ptr;
    std::mutex db_mutex;
};

static std::map<string, db_info> db_map;
static std::mutex life_cycle_mutex;

void db_writer::flush_log_list() {
    using leve_type = std::underlying_type_t<level>;
    int ret;
    db_info &info = *(static_cast<db_info *>(db_info_ptr));
    for (auto &&l : info.log_list) {
        do {
            string &module_name = l.moudle_name;
            string &comment = l.comment;
            string &data = l.data;
            ret = sqlite3_bind_int64(info.stmt_ptr, 1, l.timestamp_nano);
            if (ret != SQLITE_OK) {
                info.logger_ptr->error("bind timestamp failed", sqlite3_errmsg(info.db_ptr));
                break;
            }
            ret = sqlite3_bind_int64(info.stmt_ptr, 2, static_cast<leve_type>(l.l));
            if (ret != SQLITE_OK) {
                info.logger_ptr->error("bind level failed", sqlite3_errmsg(info.db_ptr));
                break;
            }
            ret = sqlite3_bind_text(info.stmt_ptr, 3, module_name.c_str(), module_name.length() + 1, SQLITE_STATIC);
            if (ret != SQLITE_OK) {
                info.logger_ptr->error("bind module_name failed", sqlite3_errmsg(info.db_ptr));
                break;
            }
            ret = sqlite3_bind_text(info.stmt_ptr, 4, comment.c_str(), comment.length() + 1, SQLITE_STATIC);
            if (ret != SQLITE_OK) {
                info.logger_ptr->error("bind comment failed", sqlite3_errmsg(info.db_ptr));
                break;
            }
            ret = sqlite3_bind_text(info.stmt_ptr, 5, data.c_str(), data.length() + 1, SQLITE_STATIC);
            if (ret != SQLITE_OK) {
                info.logger_ptr->error("bind data failed", sqlite3_errmsg(info.db_ptr));
                break;
            }
            ret = sqlite3_step(info.stmt_ptr);
            if (ret != SQLITE_DONE) {
                info.logger_ptr->error("step stmt failed", sqlite3_errmsg(info.db_ptr));
                break;
            }
            ret = sqlite3_reset(info.stmt_ptr);
            if (ret != SQLITE_OK) {
                info.logger_ptr->error("reset stmt failed", sqlite3_errmsg(info.db_ptr));
                break;
            }
        } while (false);
        if (ret != SQLITE_OK && ret != SQLITE_DONE) {
            std::stringstream ss;
            ss << l.timestamp_nano << " " << to_string(l.l) << " " << l.comment << " " << l.data;
            info.logger_ptr->error("insert log error", ss.str());
        }
    }
    info.log_list.clear();
}

int prepare_table_and_stmt(db_info &info) {
    constexpr char create_table[] = "create table if not exists log("
                                    "timestamp int primary key not null,"
                                    "level int,"
                                    "module_name text,"
                                    "comment text,"
                                    "data text"
                                    ");";
    constexpr char insert[] = "insert into log values(?1,?2,?3,?4,?5);";
    int ret;
    ret = sqlite3_exec(info.db_ptr, create_table, nullptr, nullptr, nullptr);
    if (ret != SQLITE_OK) {
        return ret;
    }
    ret = sqlite3_prepare_v2(info.db_ptr, insert, sizeof(insert), &info.stmt_ptr, nullptr);
    if (ret != SQLITE_OK) {
        return ret;
    }
    return SQLITE_OK;
}

db_writer::db_writer(string url, size_t buffer_szie, writer *writer_ptr) {
    std::lock_guard<std::mutex> lock{life_cycle_mutex};
    auto &info = db_map[url];
    if (info.writer_num) {
        info.writer_num++;
        info.logger_ptr->info("connect to existed db", url);
    } else {
        size_t deli = url.find_last_of('/');
        mkdir(url.substr(0, deli));
        info.url = url;
        info.writer_num = 1;
        info.logger_ptr = new log2{"log2db", writer_ptr};
        if (SQLITE_OK != sqlite3_open(url.c_str(), &info.db_ptr)) {
            info.logger_ptr->error("open db failed", sqlite3_errmsg(info.db_ptr));
            return;
        }
        if (SQLITE_OK != prepare_table_and_stmt(info)) {
            info.logger_ptr->error("prepare stmt failed", sqlite3_errmsg(info.db_ptr));
            return;
        }
        info.logger_ptr->info("open db", url);
    }
    info.buffer_size = buffer_szie;
    db_info_ptr = static_cast<void *>(&info);
}

db_writer::~db_writer() {
    std::lock_guard<std::mutex> lock{life_cycle_mutex};
    auto &info = *(static_cast<db_info *>(db_info_ptr));
    info.writer_num--;
    if (info.writer_num == 0) {
        if (!info.log_list.empty()) {
            flush_log_list();
        }
        if (SQLITE_OK != sqlite3_finalize(info.stmt_ptr)) {
            info.logger_ptr->error("finalize stmt failed", sqlite3_errmsg(info.db_ptr));
        }
        if (SQLITE_OK != sqlite3_close(info.db_ptr)) {
            info.logger_ptr->error("close db failed", sqlite3_errmsg(info.db_ptr));
        }
        info.logger_ptr->info("shut db", info.url);
        delete info.logger_ptr;
        db_map.erase(info.url);
    } else {
        info.logger_ptr->info("shut db_writer", info.url);
    }
}

void db_writer::write(level l, string module_name, string comment, string data) {
    db_info &info = *(static_cast<db_info *>(db_info_ptr));
    std::lock_guard<std::mutex> lock{info.db_mutex};
    log temp{.timestamp_nano = get_nano_timestamp(),
             .l = l,
             .comment = comment,
             .data = data};
    info.log_list.emplace_back(temp);
    if (info.log_list.size() >= info.buffer_size) {
        flush_log_list();
    }
}
