#include "./file_writer.hpp"
#include "../base/common.hpp"
#include <condition_variable>
#include <dirent.h>
#include <fstream>
#include <iomanip>
#include <list>
#include <map>
#include <mutex>
#include <regex>
#include <thread>
using namespace log2what;
using std::condition_variable;
using std::ios;
using std::list;
using std::lock_guard;
using std::map;
using std::mutex;
using std::ofstream;
using std::regex;
using std::regex_match;
using std::remove;
using std::thread;
using std::unique_lock;

static mutex dirent_mutex;

/**
 * @brief try to mimic the ls cmd
 *
 * @param path
 * @return std::list<string>
 */
inline list<string> ls(string path) {
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
    regex reg;
};

static mutex life_cycle_mutex;
static bool life_cycle_flag;
static mutex file_map_mutex;
static map<string, file_info> file_info_map;
static mutex cleaner_mutex;
static condition_variable cleaner_cv;
static thread *cleaner_ptr;

bool file_writer::open_log_file() {
    auto &info = *(static_cast<file_info *>(file_info_ptr));
    if (!info.out.is_open()) {
        // open the log file wrote last time
        auto file_list = ls(info.file_dir);
        for (auto it = file_list.rbegin(); it != file_list.rend(); it++) {
            if (regex_match(*it, info.reg)) {
                info.out.open(info.file_dir + *it, ios::app);
                return info.out.is_open();
            }
        }
    }
    if (info.out.is_open()) {
        info.out.close();
    }
    // open new log file
    constexpr int SEC_TO_MILLI = 1000;
    char buffer[21];
    int64_t timestamp_milli = get_timestamp<std::chrono::milliseconds>();
    std::tm lt = get_localtime_tm(timestamp_milli / SEC_TO_MILLI);
    std::strftime(buffer, sizeof(buffer), ".%Y%m%d_%H%M%S", &lt);
    char milli[5];
    sprintf(milli, "_%03ld", timestamp_milli % SEC_TO_MILLI);
    string file_path = info.file_dir + info.file_name;
    info.out.open(file_path.append(".log").append(buffer).append(milli), ios::app);
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
            std::cout << "in life cycle" << std::endl;
            cleaner_cv.wait(cleaner_lock);
            std::cout << "notified" << std::endl;
        }
        if (!life_cycle_flag) {
            return;
        }
        lock_guard<mutex> file_map_lock{file_map_mutex};
        map<string, list<string>> file_list_map;
        for (auto &&i : file_info_map) {
            auto file_dir = i.second.file_dir;
            if (!file_list_map.count(file_dir)) {
                file_list_map.emplace(std::move(file_dir), std::move(ls(file_dir)));
            }
            auto &file_list = file_list_map[i.second.file_dir];
            list<string>::iterator first;
            size_t count = 0;
            for (auto it = file_list.begin(); it != file_list.end(); it++) {
                if (regex_match(*it, i.second.reg)) {
                    if (!count) {
                        first = it;
                    }
                    count++;
                } else {
                    if (count) {
                        break;
                    }
                }
            }
            while (count > i.second.file_num) {
                string file_path = i.second.file_dir + *first;
                remove(file_path.c_str());
                first = file_list.erase(first);
                count--;
            }
        }
    }
}

file_writer::file_writer(string file_name, string file_dir, size_t file_size, size_t file_num) {
    const char regex_suffix[] = R"(\.log\.[0-9]{8}_[0-9]{6}_[0-9]{3})";
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
        string reg_str = file_name + regex_suffix;
        info.reg = regex{reg_str};
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
        std::cout << "life cycle false" << std::endl;
        cleaner_cv.notify_one();
        std::cout << "notify one" << std::endl;
        cleaner_ptr->join();
        delete cleaner_ptr;
        cleaner_ptr = nullptr;
    }
}

void file_writer::write(level l, string module_name, string comment, string data) {
    auto &info = *(static_cast<file_info *>(file_info_ptr));
    std::lock_guard<std::mutex> lock{info.file_mutex};
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
    int64_t timestamp_milli = get_timestamp<std::chrono::milliseconds>();
    int64_t timestamp_sec = timestamp_milli / SEC_TO_MILLI;
    int64_t precision = timestamp_milli % SEC_TO_MILLI;
    info.out << get_localtime_str(timestamp_sec)
             << "." << std::setw(3) << std::setfill('0') << precision
             << " " << to_string(l)
             << " " << module_name
             << " |%| " << comment
             << " |%| " << data << std::endl;
}