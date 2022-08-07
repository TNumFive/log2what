#include "./db_writer.hpp"
#include <list>
#include <map>
#include <sqlite3.h>
#include <thread>

using namespace log2what;

/**
 * @brief make dir with cmd
 *
 * @param dir_path
 */
inline void mkdir(string dir_path) {
#ifdef __WIN32__
    string cmd = "if not exist \"${dir}\" (md \"${dir}\")";
    dir_path.replace(dir_path.begin(), dir_path.end(), '/', '\\');
#else
    string cmd = "if [ ! -d \"${dir}\" ]; then \n\tmkdir -p \"${dir}\"\nfi";
#endif
    cmd.replace(cmd.find("${dir}"), 6, dir_path);
    cmd.replace(cmd.find("${dir}"), 6, dir_path);
    system(cmd.c_str());
}

struct log {
    int64_t timestamp_nano;
    string moudle_name;
    string comment;
    string data;
};

struct db_info {
    string url;
    size_t counter;
    sqlite3 *db_ptr;
    sqlite3_stmt *stmt_ptr;
    std::list<log> log_list;
    size_t buffer_size;
    log2 *logger_ptr;
    std::mutex db_mutex;
};

static std::map<string, db_info> db_map;
static std::mutex life_cycle_mutex;

int prepare_table_and_stmt(db_info &info) {
    constexpr char create_table[] = "create table if not exists log("
                                    "timestamp int primary key not null,"
                                    "module_name text,"
                                    "comment text,"
                                    "data text"
                                    ");";
    constexpr char insert[] = "insert into log values(?1,?2,?3,?4);";
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

db_writer::db_writer(string url, writer *writer_ptr) {
    this->url = url;
    std::lock_guard<std::mutex> lock{life_cycle_mutex};
    auto &info = db_map[url];
    if (info.counter) {
        info.counter++;
        info.logger_ptr->info("connect to existed db", url);
    } else {
        size_t deli = url.find_last_of('/');
        mkdir(url.substr(0, deli));
        info.counter = 1;
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
    db_info_ptr = static_cast<void *>(&info);
}

db_writer::~db_writer() {
    std::lock_guard<std::mutex> lock{life_cycle_mutex};
    auto &info = db_map[url];
    info.counter--;
    if (info.counter == 0) {
        if (SQLITE_OK != sqlite3_finalize(info.stmt_ptr)) {
            info.logger_ptr->error("finalize stmt failed", sqlite3_errmsg(info.db_ptr));
        }
        if (SQLITE_OK != sqlite3_close(info.db_ptr)) {
            info.logger_ptr->error("close db failed", sqlite3_errmsg(info.db_ptr));
        }
        info.logger_ptr->info("shut db", url);
        delete info.logger_ptr;
        db_map.erase(url);
    } else {
        info.logger_ptr->info("shut db_writer", url);
    }
}

void db_writer::write(level l, string module_name, string comment, string data) {
    db_info &info = *(static_cast<db_info *>(db_info_ptr));
    int ret;
    std::lock_guard<std::mutex> lock{info.db_mutex};
    do {
        ret = sqlite3_bind_int64(info.stmt_ptr, 1, get_nano_timestamp());
        if (ret != SQLITE_OK) {
            info.logger_ptr->error("bind ts failed", sqlite3_errmsg(info.db_ptr));
            break;
        }
        ret = sqlite3_bind_text(info.stmt_ptr, 2, module_name.c_str(), module_name.length() + 1, SQLITE_STATIC);
        if (ret != SQLITE_OK) {
            info.logger_ptr->error("bind module_name failed", sqlite3_errmsg(info.db_ptr));
            break;
        }
        ret = sqlite3_bind_text(info.stmt_ptr, 3, comment.c_str(), comment.length() + 1, SQLITE_STATIC);
        if (ret != SQLITE_OK) {
            info.logger_ptr->error("bind comment failed", sqlite3_errmsg(info.db_ptr));
            break;
        }
        ret = sqlite3_bind_text(info.stmt_ptr, 4, data.c_str(), data.length() + 1, SQLITE_STATIC);
        if (ret != SQLITE_OK) {
            info.logger_ptr->error("bind data failed", sqlite3_errmsg(info.db_ptr));
            break;
        }
        ret = sqlite3_step(info.stmt_ptr);
        if (ret != SQLITE_DONE) {
            info.logger_ptr->error("step stmt failed", sqlite3_errmsg(info.db_ptr));
        }
        ret = sqlite3_reset(info.stmt_ptr);
        if (ret != SQLITE_OK) {
            info.logger_ptr->error("reset stmt failed", sqlite3_errmsg(info.db_ptr));
        }
    } while (false);
}
