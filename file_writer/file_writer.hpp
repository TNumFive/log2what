#ifndef FILE_WRITER_HPP
#define FILE_WRITER_HPP

#include "../base/common.hpp"
#include "../base/writer.hpp"
#include <condition_variable>
#include <fstream>
#include <list>
#include <map>
#include <mutex>
#include <thread>

namespace log2what {
/**
 * @brief make dir by exec-cmd
 *
 * @param dir_path
 */
void mkdir(string dir_path);

/**
 * @brief try to mimic the ls cmd with dirent
 *
 * @param path
 * @return std::list<string>
 */
std::list<string> ls(string path);

static constexpr size_t KB = 1024;
static constexpr size_t MB = 1024 * KB;

struct file_info {
    size_t writer_num;
    std::mutex file_mutex;
    std::ofstream out;
    string file_dir;
    string file_name;
    size_t file_size;
    size_t file_num;
};

struct file_writer_config {
    string file_name{"root"};
    string file_dir{"./log/"};
    size_t file_size{MB};
    size_t file_num{50};
};

class file_writer : public writer {
  private:
    static std::mutex file_map_mutex;
    static std::map<string, file_info> file_map;
    static std::mutex cleaner_mutex;
    static std::condition_variable cleaner_cv;
    static std::thread *cleaner;
    file_info *fip;
    virtual bool open_log_file();
    virtual void clean_log_file();

  public:
    file_writer(string file_name = "root", string file_dir = "./log/", size_t file_size = MB, size_t file_num = 50);
    file_writer(file_writer_config &fwc);
    virtual ~file_writer() override;
    void write(level l, string module_name, string comment, string data) override;
};
} // namespace log2what

#endif