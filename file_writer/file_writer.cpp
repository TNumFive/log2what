#include "./file_writer.hpp"
#include "../base/common.hpp"
#include <dirent.h>
#include <fstream>
#include <iomanip>
#include <map>
#include <mutex>
#include <set>
using namespace log2what;
using namespace std;
using std::chrono::milliseconds;

struct file_info
{
    mutex file_mutex;
    ofstream out;
    size_t writer_num;
    string file_dir;
    string file_name;
    size_t file_size;
    size_t file_num;
    bool keep_alive;
    string map_key;
};

static mutex life_cycle_mutex;
static map<string, file_info> file_info_map;
constexpr char file_name_deli[] = ".log";

/**
 * @brief gen log file suffix
 *
 * @return string like .20220814_105832_204
 */
inline string gen_log_file_suffix()
{
    constexpr int sec_to_milli = 1000;
    char buffer[21];
    int64_t timestamp_milli = get_timestamp<milliseconds>();
    tm lt = get_localtime_tm(timestamp_milli / sec_to_milli);
    strftime(buffer, sizeof(buffer), ".%Y%m%d_%H%M%S", &lt);
    sprintf(&buffer[16], "_%03ld", timestamp_milli % sec_to_milli);
    return buffer;
}

/**
 * @brief hard-code log_file name format check fn
 *
 * @param file
 * @return true
 * @return false
 */
inline bool is_log_file(const char *file_suffix)
{
    constexpr char low[] = ".log.00000000_000000_000";
    constexpr char top[] = ".log.99991939_295959_999";
    for (size_t i = 0; i < sizeof(low); i++)
    {
        if (file_suffix[i] < low[i] || file_suffix[i] > top[i])
        {
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
inline bool is_log_file(const string &name, const string &file)
{
    size_t size = name.size();
    for (size_t i = 0; i < size; i++)
    {
        if (name[i] != file[i])
        {
            return false;
        }
    }
    return is_log_file(&(file.c_str()[size]));
}

inline set<string> filtered_ls(const string &name, const string &path)
{
    DIR *dir;
    dirent64 *diread;
    dir = opendir(path.c_str());
    if (dir != nullptr)
    {
        set<string> file_set;
        diread = readdir64(dir);
        while (diread != nullptr)
        {
            if (is_log_file(name, diread->d_name))
            {
                file_set.insert(diread->d_name);
            }
            diread = readdir64(dir);
        }
        closedir(dir);
        return file_set;
    }
    return {};
}

bool file_writer::open_log_file()
{
    auto &info = *(static_cast<file_info *>(file_info_ptr));
    auto file_set = filtered_ls(info.file_name, info.file_dir);
    if (!info.out.is_open() && file_set.size())
    {
        // this baiscally happend only once during the lifecycle
        // open the log file wrote last time
        info.out.open(info.file_dir + *(file_set.rbegin()), ios::app);
        return info.out.is_open();
    }
    info.out.close();
    // open new log file
    string new_path = info.map_key;
    new_path.append(file_name_deli).append(gen_log_file_suffix());
    if (file_set.size() < info.file_num)
    {
        info.out.open(new_path, ios::app);
    }
    else
    {
        // remove extra files
        while (file_set.size() > info.file_num)
        {
            string file_path = info.file_dir + *(file_set.begin());
            remove(file_path.c_str());
            file_set.erase(*(file_set.begin()));
        }
        // rename the oldest file
        string old_path = info.file_dir + *(file_set.begin());
        rename(old_path.c_str(), new_path.c_str());
        info.out.open(new_path, ios::trunc);
    }
    return info.out.is_open();
}

file_writer::file_writer(const string &file_name, const string &file_dir,
                         const size_t file_size, const size_t file_num,
                         const bool keep_alive)
{
    string map_key = file_dir + file_name;
    lock_guard<mutex> life_cycle_lock{life_cycle_mutex};
    auto &info = file_info_map[map_key];
    file_info_ptr = static_cast<void *>(&info);
    info.writer_num++;
    info.file_size = file_size > 1 ? file_size : 1;
    info.file_num = file_num;
    info.keep_alive = keep_alive;
    lock_guard<mutex> lock{info.file_mutex};
    if (!info.out.is_open())
    {
        info.file_dir = file_dir;
        info.file_name = file_name;
        info.map_key = std::move(map_key);
        auto dir_ptr = opendir(file_dir.c_str());
        if (dir_ptr == nullptr)
        {
            mkdir(file_dir);
        }
        else
        {
            closedir(dir_ptr);
        }
        open_log_file();
    }
}

file_writer::~file_writer()
{
    auto &info = *(static_cast<file_info *>(file_info_ptr));
    lock_guard<mutex> life_cycle_lock{life_cycle_mutex};
    info.writer_num--;
    if (info.writer_num == 0 && !info.keep_alive)
    {
        string map_key = std::move(info.map_key);
        file_info_map.erase(map_key);
    }
}

void file_writer::write(const log_level level, const string &module_name,
                        const string &comment, const string &data,
                        const int64_t timestamp_nano)
{
    constexpr int sec_to_milli = 1000;
    constexpr int milli_to_nano = 1000000;
    int64_t timestamp_milli = timestamp_nano
                                  ? timestamp_nano / milli_to_nano
                                  : get_timestamp<milliseconds>();
    int64_t timestamp_sec = timestamp_milli / sec_to_milli;
    int64_t precision = timestamp_milli % sec_to_milli;
    constexpr int fixed_length = sizeof("2022-07-30 17:02:38.795 T |%|  |%| \n");
    auto &info = *(static_cast<file_info *>(file_info_ptr));
    size_t size_to_write = fixed_length +
                           module_name.length() +
                           comment.length() + data.length();
    lock_guard<mutex> lock{info.file_mutex};
    if (info.out.tellp() > info.file_size - size_to_write)
    {
        // exceed size, open new log file
        if (!open_log_file())
        {
            std::cerr << "log2what::file_writer open file failed" << std::endl;
            return;
        }
    }
    info.out << get_localtime_str(timestamp_sec)
             << "." << setw(3) << setfill('0') << precision
             << " " << to_string(level)
             << " " << module_name
             << " |%| " << comment
             << " |%| " << data << "\n";
}