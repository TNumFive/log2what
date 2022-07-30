#include "./file_writer.hpp"
#include <dirent.h>
using namespace log2what;

std::mutex file_writer::file_map_mutex;
std::map<string, file_info> file_writer::file_map;

bool file_writer::open_log_file() {
    string file_path = file_dir + file_name;
    if (file_size <= 1) {
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
        // todo: try remove extra file
    }
    return fip->out.is_open();
}

file_writer::file_writer(string file_name, string file_dir, size_t file_size, size_t file_num) {
    this->file_dir = file_dir;
    this->file_name = file_name;
    this->file_size = file_size;
    this->file_num = file_num;
    string map_key = file_dir + file_name;
    std::unique_lock<std::mutex> m_lock{file_map_mutex};
    mkdir(this->file_dir);
    fip = &file_map[map_key];
    m_lock.unlock();
    std::lock_guard<std::mutex> f_lock{fip->file_mutex};
    fip->writer_num++;
    if (fip->writer_num == 1) {
        if (!open_log_file()) {
            throw "open log file failed";
        }
    }
}

file_writer::~file_writer() {
    std::lock_guard<std::mutex> lock{this->file_map_mutex};
    string map_key = file_dir + file_name;
    file_info &fi = file_map[map_key];
    fi.writer_num--;
    if (fi.writer_num == 0) {
        file_map.erase(map_key);
    }
}

void file_writer::write(level l, string module_name, string comment, string data) {
    std::lock_guard<std::mutex> lock{fip->file_mutex};
    // note: dead_length depens on the format wrote to log
    // like siezeof("2022-07-30 17:02:38.795 TRACE  |%|  |%| \n")
    constexpr int dead_length = 41;
    size_t size_to_write = dead_length + module_name.length() + comment.length() + data.length();
    if (fip->out.tellp() > file_size - size_to_write) {
        // exceed size, open new log file
        fip->out.close();
        if (!open_log_file()) {
            throw "open log file failed";
        }
        // remove extra files if exceed file_num
        
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