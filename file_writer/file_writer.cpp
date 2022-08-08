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

static std::mutex dirent_mutex;

/**
 * @brief try to mimic the ls cmd
 *
 * @param path
 * @return std::list<string>
 */
inline std::list<string> ls(string path) {
    DIR *dir;
    dirent64 *diread;
    std::lock_guard<std::mutex> lock{dirent_mutex};
    dir = opendir(path.c_str());
    if (dir != nullptr) {
        std::list<string> file_list;
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
    size_t writer_num;
    std::mutex file_mutex;
    std::ofstream out;
    string file_dir;
    string file_name;
    size_t file_size;
    size_t file_num;
};

bool life_cycle_flag{true};
std::mutex life_cycle_mutex;
std::mutex file_map_mutex;
std::map<string, file_info> file_map;
std::mutex cleaner_mutex;
std::condition_variable cleaner_cv;
std::thread *cleaner = nullptr;

bool file_writer::open_log_file() {
    file_info &info = *(static_cast<file_info *>(file_info_ptr));
    string file_path = info.file_dir + info.file_name;
    if (info.file_size <= 1) {
        info.out.open(file_path.append(".log"), std::ios_base::trunc);
    } else {
        constexpr int SEC_TO_MILLI = 1000;
        char buffer[21];
        int64_t timestamp_milli = get_timestamp<std::chrono::milliseconds>();
        std::tm lt = get_localtime_tm(timestamp_milli / SEC_TO_MILLI);
        std::strftime(buffer, sizeof(buffer), ".%Y%m%d_%H%M%S", &lt);
        char milli[5];
        sprintf(milli, "_%03ld", timestamp_milli % SEC_TO_MILLI);
        info.out.open(file_path.append(".log").append(buffer).append(milli),
                      std::ios_base::ate | std::ios::app);
    }
    return info.out.is_open();
}

/**
 * @brief function called periodically to clear extra log file
 *
 */
void file_writer::clean_log_file() {
    while (true) {
        std::unique_lock<std::mutex> lock{cleaner_mutex};
        if (life_cycle_flag) {
            cleaner_cv.wait(lock);
        } else {
            return;
        }
        std::lock_guard<std::mutex> m_lock{file_map_mutex};
        std::map<string, std::list<string>> file_list_map;
        for (auto &&fi : file_map) {
            string &file_dir = fi.second.file_dir;
            if (file_list_map.count(file_dir) == 0) {
                file_list_map.emplace(file_dir, ls(file_dir));
            }
            auto &file_list = file_list_map[file_dir];
            size_t file_num = 0;
            std::list<string>::iterator first = file_list.end();
            // calculate the nums, record the first one
            for (auto it = file_list.begin(); it != file_list.end(); it++) {
                if (it->find(fi.second.file_name) != string::npos) {
                    file_num++;
                    if (first == file_list.end()) {
                        first = it;
                    }
                }
            }
            while (file_num > fi.second.file_num) {
                string file_to_del = fi.second.file_dir + (*first);
                remove(file_to_del.c_str());
                first = file_list.erase(first);
                file_num--;
            }
        }
    }
}

file_writer::file_writer(string file_name, string file_dir, size_t file_size, size_t file_num) {
    string map_key = file_dir + file_name;
    std::lock_guard<std::mutex> life_cycle_lock{life_cycle_mutex};
    life_cycle_flag = true;
    std::lock_guard<std::mutex> m_lock{file_map_mutex};
    mkdir(file_dir);
    file_info_ptr = &file_map[map_key];
    file_info &info = file_map[map_key];
    if (cleaner == nullptr) {
        cleaner = new std::thread{&file_writer::clean_log_file, this};
    }
    info.file_size = file_size;
    info.file_num = file_num <= 1 ? 1 : file_num;
    info.writer_num++;
    if (info.writer_num == 1) {
        info.file_name = file_name;
        info.file_dir = file_dir;
        if (!open_log_file()) {
            throw "open log file failed";
        }
        cleaner_cv.notify_one();
    }
}

file_writer::~file_writer() {
    file_info &info = *(static_cast<file_info *>(file_info_ptr));
    string map_key = info.file_dir + info.file_name;
    std::lock_guard<std::mutex> life_cycle_lock{life_cycle_mutex};
    std::unique_lock<std::mutex> m_lock{file_map_mutex};
    info.writer_num--;
    if (info.writer_num == 0) {
        file_map.erase(map_key);
    }
    if (file_map.size() == 0) {
        life_cycle_flag = false;
        m_lock.unlock();
        cleaner_cv.notify_one();
        cleaner->join();
        delete cleaner;
        cleaner = nullptr;
    }
}

void file_writer::write(level l, string module_name, string comment, string data) {
    file_info &info = *(static_cast<file_info *>(file_info_ptr));
    std::lock_guard<std::mutex> lock{info.file_mutex};
    // note: dead_length depens on the format wrote to log
    // like sizeof("2022-07-30 17:02:38.795 TRACE  |%|  |%| \n")
    constexpr int dl = sizeof("2022-07-30 17:02:38.795 TRACE |%|  |%| \n");
    size_t size_to_write = dl + module_name.length() + comment.length() + data.length();
    if (info.out.tellp() > info.file_size - size_to_write) {
        // exceed size, open new log file
        info.out.close();
        if (!open_log_file()) {
            throw "open log file failed";
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