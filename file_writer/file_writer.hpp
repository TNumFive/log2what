#ifndef FILE_WRITER_HPP
#define FILE_WRITER_HPP

#include "../base/common.hpp"
#include "../base/writer.hpp"
#include <condition_variable>
#include <fstream>
#include <map>
#include <mutex>
#include <thread>

namespace log2what {
struct file_info {
    size_t writer_num;
    std::mutex file_mutex;
    std::ofstream out;
    string file_dir;
    string file_name;
    size_t file_size;
    size_t file_num;
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
    static constexpr size_t KB = 1024;
    static constexpr size_t MB = 1024 * KB;
    file_writer(string file_name = "root", string file_dir = "./log/", size_t file_size = MB, size_t file_num = 50);
    virtual ~file_writer() override;
    void write(level l, string module_name, string comment, string data) override;
};
} // namespace log2what

#endif