/**
 * @file file_writer.cpp
 * @author TNumFive
 * @brief Example of file writer.
 * @version 0.1
 * @date 2023-01-13
 *
 * @copyright Copyright (c) 2023
 *
 */
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

constexpr char log_extension[] = ".log";

/**
 * @brief File helper for open, write, remove log files.
 */
class file_helper
{
public:
    /**
     * @brief Construct a new file helper object.
     *
     * @param file_dir Directory for log file.
     * @param file_name Name of log file.
     * @param file_size The max size of log file.
     * @param file_num The file name of log file rotation.
     */
    file_helper(const string &file_dir, const string &file_name,
                const size_t file_size, size_t file_num)
    {
        mkdir(file_dir);
        this->file_dir = file_dir;
        this->file_name = file_name;
        this->file_size = file_size;
        this->file_num = file_num;
        lock_guard<mutex> file_lock{this->file_mutex};
        this->open_log_file();
    }
    /**
     * @brief Default destructor.
     */
    ~file_helper() = default;
    /**
     * @brief Write log to file.
     *
     * @param level Log level.
     * @param module Module name.
     * @param comment Content of log.
     * @param data Data attached.
     * @param timestamp_nano Timestamp of log in nanoseconds.
     */
    void write(const log_level level, const string &module,
               const string &comment, const string &data,
               const int64_t timestamp_nano)
    {
        constexpr int sec_to_milli = 1000;
        constexpr int milli_to_nano = 1000000;
        constexpr char fixed_part[] = "2022-07-30 17:02:38.795 T |%|  |%| \n";
        constexpr int fixed_length = sizeof(fixed_part);
        int64_t timestamp_milli = timestamp_nano / milli_to_nano;
        int64_t timestamp_sec = timestamp_milli / sec_to_milli;
        int64_t precision = timestamp_milli % sec_to_milli;
        size_t size_to_write = fixed_length;
        size_to_write += module.length();
        size_to_write += comment.length();
        size_to_write += data.length();
        lock_guard<mutex> file_lock{this->file_mutex};
        if (this->out.tellp() > this->file_size - size_to_write)
        {
            if (!this->open_log_file())
            {
                std::cerr << "log2what::file_writer open file failed";
                std::cerr << std::endl;
                return;
            }
        }
        this->out << get_localtime_str(timestamp_sec);
        this->out << "." << setw(3) << setfill('0') << precision;
        this->out << " " << to_string(level);
        this->out << " " << module;
        this->out << " |%| " << comment;
        this->out << " |%| " << data << "\n";
    }

private:
    ofstream out;
    string file_dir;
    string file_name;
    size_t file_size;
    size_t file_num;
    mutex file_mutex;
    /**
     * @brief Generate log file suffix.
     *
     * @return string Generated suffix string.
     */
    inline string generate_log_file_suffix()
    {
        constexpr int sec_to_milli = 1000;
        char buffer[21];
        int64_t timestamp = get_timestamp<milliseconds>();
        tm lt = get_localtime_tm(timestamp / sec_to_milli);
        strftime(buffer, sizeof(buffer), ".%Y%m%d_%H%M%S", &lt);
        sprintf(&buffer[16], "_%03ld", timestamp % sec_to_milli);
        return buffer;
    }
    /**
     * @brief Check if suffix conform to special format.
     *
     * @param file_suffix Given suffix of file name.
     * @return true Yes.
     * @return false No.
     */
    inline bool is_log_file_suffix(const char *file_suffix)
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
     * @brief Check if given file is log file of certain file writer.
     *
     * @param name The file_name of file writer.
     * @param file_name File name of given file to check.
     * @return true Yes.
     * @return false No.
     */
    inline bool is_log_file(const string &name, const string &file_name)
    {
        size_t size = name.size();
        for (size_t i = 0; i < size; i++)
        {
            if (name[i] != file_name[i])
            {
                return false;
            }
        }
        return is_log_file_suffix(&(file_name.c_str()[size]));
    }
    /**
     * @brief Filter out all log files in given path.
     *
     * @param name The file_name of file writer.
     * @param path Given path of director.
     * @return set<string> Set of log file names.
     */
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
    /**
     * @brief Open log file.
     * 
     * @details Open old log file if exists when fstream is closed; Open new
     * file if log file max nums not exceeded; Reuse and rename old log file if
     * log file max nums met.
     * 
     * @return true If open log file succeeded.
     * @return false If open log file failed.
     */
    bool open_log_file()
    {
        auto file_set = this->filtered_ls(this->file_name, this->file_dir);
        if (!this->out.is_open() && file_set.size())
        {
            // file not opened but old log files already exist.
            this->out.open(this->file_dir + *(file_set.rbegin()), ios::app);
            return this->out.is_open();
        }
        // file opened, close file and try open new file.
        if (this->out.is_open())
        {
            this->out.close();
        }
        string new_path = this->file_dir + this->file_name;
        new_path.append(log_extension).append(this->generate_log_file_suffix());
        if (file_set.size() < this->file_num)
        {
            this->out.open(new_path, ios::app);
        }
        else
        {
            while (file_set.size() > this->file_num)
            {
                string file_path = this->file_dir + *file_set.begin();
                remove(file_path.c_str());
                file_set.erase(file_set.begin());
            }
            string old_path = this->file_dir + *file_set.begin();
            rename(old_path.c_str(), new_path.c_str());
            this->out.open(new_path, ios::trunc);
        }
        return out.is_open();
    }
};

static mutex life_cycle_mutex;
/**
 * @brief Map of file helper for different log files.
 * 
 * @details Make sure only one helper for one {file_dir + file_name}.
 */
static map<string, unique_ptr<file_helper>> helper_map;

/**
 * @brief Construct a new file writer object.
 *
 * @param file_name Log file name.
 * @param file_dir Directory for log files.
 * @param file_size The max log file size.
 * @param file_num The number of log file rotation.
 */
file_writer::file_writer(const string &file_name, const string &file_dir,
                         const size_t file_size, const size_t file_num)
{
    this->helper_map_key = file_dir + file_name;
    lock_guard<mutex> life_cycle_lock{life_cycle_mutex};
    if (helper_map.count(this->helper_map_key))
    {
        return;
    }
    auto file_helper_ptr =
        new file_helper{file_dir, file_name, file_size, file_num};
    helper_map[this->helper_map_key].reset(file_helper_ptr);
}

/**
 * @brief Write log to file.
 *
 * @param level Log level.
 * @param module Module name.
 * @param comment Content of log.
 * @param data Data attached.
 * @param timestamp_nano Timestamp of log in nanoseconds.
 */
void file_writer::write(const log_level level, const string &module,
                        const string &comment, const string &data,
                        const int64_t timestamp_nano)
{
    int64_t timestamp = timestamp_nano;
    if (!timestamp)
    {
        timestamp = get_nano_timestamp();
    }
    lock_guard<mutex> life_cycle_lock{life_cycle_mutex};
    auto &helper = helper_map[this->helper_map_key];
    helper->write(level, module, comment, data, timestamp);
}