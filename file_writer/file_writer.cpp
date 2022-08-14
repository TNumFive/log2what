#include "./file_writer.hpp"
#include "../base/common.hpp"
#include <condition_variable>
#include <dirent.h>
#include <fstream>
#include <iomanip>
#include <list>
#include <map>
#include <mutex>
#include <thread>
using namespace log2what;
using namespace std;
using std::chrono::milliseconds;

static mutex dirent_mutex;

/**
 * @brief try to mimic the ls cmd
 *
 * @param path
 * @return list<string>
 */
inline list<string> ls(const string &path) {
    DIR *dir;
    dirent64 *diread;
    lock_guard<mutex> lock{dirent_mutex};
    dir = opendir(path.c_str());
    if (dir != nullptr) {
        list<string> file_list;
        diread = readdir64(dir);
        while (diread != nullptr) {
            file_list.push_back(diread->d_name);
            diread = readdir64(dir);
        }
        closedir(dir);
        file_list.sort();
        return file_list;
    }
    return {};
}

struct file_info {
    mutex file_mutex;
    ofstream out;
    size_t writer_num;
    string file_dir;
    string file_name;
    size_t file_size;
    size_t file_num;
};

static mutex life_cycle_mutex;
static bool life_cycle_flag;
static mutex file_map_mutex;
static map<string, file_info> file_info_map;
static mutex cleaner_mutex;
static condition_variable cleaner_cv;
static thread *cleaner_ptr;
constexpr char file_name_deli[] = ".log.";

/**
 * @brief append log file suffix to file_path
 *
 * @param file_path file_dir+file_name
 * @return string& file_dir+file_name+file_name_deli+time_str
 */
inline string &append_log_file_suffix(string &file_path) {
    constexpr int SEC_TO_MILLI = 1000;
    char buffer[21];
    int64_t timestamp_milli = get_timestamp<milliseconds>();
    tm lt = get_localtime_tm(timestamp_milli / SEC_TO_MILLI);
    strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &lt);
    char milli[5];
    sprintf(milli, "_%03ld", timestamp_milli % SEC_TO_MILLI);
    file_path.append(file_name_deli).append(buffer).append(milli);
    return file_path;
}

/**
 * @brief hard-code log_file name format check fn
 *
 * @param file
 * @return true
 * @return false
 */
inline bool is_log_file(const char *file_suffix) {
    constexpr char low[] = ".log.00000000_000000_000";
    constexpr char top[] = ".log.99991939_295959_999";
    for (size_t i = 0; i < sizeof(low); i++) {
        if (file_suffix[i] < low[i] || file_suffix[i] > top[i]) {
            return false;
        }
    }
    return true;
}

/**
 * @brief hard-code log_file name format check fn
 *
 * @param file
 * @return true
 * @return false
 */
inline bool is_log_file(const string &name, const string &file) {
    size_t size = name.size();
    for (size_t i = 0; i < size; i++) {
        if (name[i] != file[i]) {
            return false;
        }
    }
    return is_log_file(&(file.c_str()[size]));
}

bool file_writer::open_log_file() {
    auto &info = *(static_cast<file_info *>(file_info_ptr));
    if (!info.out.is_open()) {
        // open the log file wrote last time
        auto file_list = ls(info.file_dir);
        for (auto it = file_list.rbegin(); it != file_list.rend(); it++) {
            if (is_log_file(info.file_name, *it)) {
                info.out.open(info.file_dir + *it, ios::app);
                return info.out.is_open();
            }
        }
    }
    if (info.out.is_open()) {
        info.out.close();
    }
    // open new log file
    string file_path = info.file_dir + info.file_name;
    info.out.open(append_log_file_suffix(file_path), ios::app);
    return info.out.is_open();
}

/**
 * @brief function called periodically to clear extra log file
 *
 */
void file_writer::clean_log_file() {
    while (life_cycle_flag) {
        unique_lock<mutex> cleaner_lock{cleaner_mutex};
        if (life_cycle_flag) {
            cleaner_cv.wait(cleaner_lock);
        }
        if (!life_cycle_flag) {
            return;
        }
        lock_guard<mutex> file_map_lock{file_map_mutex};
        // map<file_dir,map<file_name,list<file>>>
        map<string, map<string, list<string>>> dir_file_list_map;
        for (auto &&i : file_info_map) {
            auto &file_dir = i.second.file_dir;
            if (!dir_file_list_map.count(file_dir)) {
                auto &file_list_map = dir_file_list_map[file_dir];
                auto file_list = ls(file_dir);
                for (auto &&file : file_list) {
                    auto deli_pos = file.find(file_name_deli);
                    if (deli_pos != string::npos && is_log_file(&(file.c_str()[deli_pos]))) {
                        string file_name = file.substr(0, deli_pos);
                        file_list_map[file_name].push_back(file);
                    }
                }
            }
            auto &file_list = dir_file_list_map[file_dir][i.second.file_name];
            while (file_list.size() > i.second.file_num) {
                string file_path{file_dir + file_list.front()};
                remove(file_path.c_str());
                file_list.pop_front();
            }
        }
    }
}

file_writer::file_writer(const string &file_name, const string &file_dir, size_t file_size, size_t file_num) {
    string map_key = file_dir + file_name;
    lock_guard<mutex> life_cycle_lock{life_cycle_mutex};
    if (cleaner_ptr == nullptr) {
        cleaner_ptr = new thread{&file_writer::clean_log_file, this};
    }
    lock_guard<mutex> file_map_lock{file_map_mutex};
    life_cycle_flag = true;
    auto &info = file_info_map[map_key];
    file_info_ptr = &info;
    if (info.writer_num == 0) {
        mkdir(file_dir);
        info.file_dir = file_dir;
        info.file_name = file_name;
        open_log_file();
        cleaner_cv.notify_one();
    }
    info.file_size = file_size > 1 ? file_size : 1;
    info.file_num = file_num;
    info.writer_num++;
}

file_writer::~file_writer() {
    auto &info = *(static_cast<file_info *>(file_info_ptr));
    lock_guard<mutex> life_cycle_lock{life_cycle_mutex};
    unique_lock<mutex> file_map_lock{file_map_mutex};
    info.writer_num--;
    if (info.writer_num == 0) {
        string map_key = info.file_dir + info.file_name;
        file_info_map.erase(map_key);
    }
    file_map_lock.unlock();
    if (file_info_map.empty()) {
        unique_lock<mutex> cleaner_lock{cleaner_mutex};
        life_cycle_flag = false;
        cleaner_lock.unlock();
        cleaner_cv.notify_one();
        cleaner_ptr->join();
        delete cleaner_ptr;
        cleaner_ptr = nullptr;
    }
}

void file_writer::write(level l, string module_name, string comment, string data) {
    auto &info = *(static_cast<file_info *>(file_info_ptr));
    lock_guard<mutex> lock{info.file_mutex};
    constexpr int fixed_log_length = sizeof("2022-07-30 17:02:38.795 TRACE |%|  |%| \n");
    size_t size_to_write = fixed_log_length + module_name.length() + comment.length() + data.length();
    if (info.out.tellp() > info.file_size - size_to_write) {
        // exceed size, open new log file
        if (!open_log_file()) {
            return;
        }
        cleaner_cv.notify_one();
    }
    constexpr int SEC_TO_MILLI = 1000;
    int64_t timestamp_milli = get_timestamp<milliseconds>();
    int64_t timestamp_sec = timestamp_milli / SEC_TO_MILLI;
    int64_t precision = timestamp_milli % SEC_TO_MILLI;
    info.out << get_localtime_str(timestamp_sec)
             << "." << setw(3) << setfill('0') << precision
             << " " << to_string(l)
             << " " << module_name
             << " |%| " << comment
             << " |%| " << data << "\n";
}