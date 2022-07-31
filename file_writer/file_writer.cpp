#include "./file_writer.hpp"
#include <dirent.h>
#include <list>
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

std::mutex file_writer::file_map_mutex;
std::map<string, file_info> file_writer::file_map;
std::mutex file_writer::cleaner_mutex;
std::condition_variable file_writer::cleaner_cv;
std::thread *file_writer::cleaner = nullptr;

bool file_writer::open_log_file() {
    string file_path = fip->file_dir + fip->file_name;
    if (fip->file_size <= 1) {
        fip->out.open(file_path.append(".log"), std::ios_base::trunc);
    } else {
        constexpr int SEC_TO_MILLI = 1000;
        char buffer[21];
        int64_t timestamp_milli = get_timestamp<std::chrono::milliseconds>();
        std::tm lt = get_localtime_tm(timestamp_milli / SEC_TO_MILLI);
        std::strftime(buffer, sizeof(buffer), ".%Y%m%d_%H%M%S", &lt);
        char milli[5];
        sprintf(milli, "_%03ld", timestamp_milli % SEC_TO_MILLI);
        fip->out.open(file_path.append(".log").append(buffer).append(milli),
                      std::ios_base::ate | std::ios::app);
    }
    return fip->out.is_open();
}

/**
 * @brief function called periodically to clear extra log file
 *
 */
void file_writer::clean_log_file() {
    while (true) {
        std::unique_lock<std::mutex> lock{cleaner_mutex};
        cleaner_cv.wait(lock);
        if (file_map.size() == 0) {
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
    std::unique_lock<std::mutex> m_lock{file_map_mutex};
    mkdir(file_dir);
    fip = &file_map[map_key];
    if (cleaner == nullptr) {
        cleaner = new std::thread{&file_writer::clean_log_file, this};
    }
    m_lock.unlock();
    std::lock_guard<std::mutex> f_lock{fip->file_mutex};
    fip->file_size = file_size;
    fip->file_num = file_num <= 1 ? 1 : file_num;
    fip->writer_num++;
    if (fip->writer_num == 1) {
        fip->file_name = file_name;
        fip->file_dir = file_dir;
        if (!open_log_file()) {
            throw "open log file failed";
        }
    }
}

file_writer::~file_writer() {
    string map_key = fip->file_dir + fip->file_name;
    std::lock_guard<std::mutex> m_lock{this->file_map_mutex};
    std::unique_lock<std::mutex> f_lock{fip->file_mutex};
    fip->writer_num--;
    if (fip->writer_num == 0) {
        f_lock.unlock();
        file_map.erase(map_key);
    }
    if (file_map.size() == 0) {
        cleaner_cv.notify_one();
        cleaner->join();
        delete cleaner;
        cleaner = nullptr;
    }
}

void file_writer::write(level l, string module_name, string comment, string data) {
    std::lock_guard<std::mutex> lock{fip->file_mutex};
    // note: dead_length depens on the format wrote to log
    // like sizeof("2022-07-30 17:02:38.795 TRACE  |%|  |%| \n")
    constexpr int dl = sizeof("2022-07-30 17:02:38.795 TRACE |%|  |%| \n");
    size_t size_to_write = dl + module_name.length() + comment.length() + data.length();
    if (fip->out.tellp() > fip->file_size - size_to_write) {
        // exceed size, open new log file
        fip->out.close();
        if (!open_log_file()) {
            throw "open log file failed";
        }
        cleaner_cv.notify_one();
    }
    constexpr int SEC_TO_MILLI = 1000;
    int64_t timestamp_milli = get_timestamp<std::chrono::milliseconds>();
    int64_t timestamp_sec = timestamp_milli / SEC_TO_MILLI;
    int64_t precision = timestamp_milli % SEC_TO_MILLI;
    fip->out << get_localtime_str(timestamp_sec)
             << "." << std::setw(3) << std::setfill('0') << precision
             << " " << to_string(l)
             << " " << module_name
             << " |%| " << comment
             << " |%| " << data << std::endl;
}