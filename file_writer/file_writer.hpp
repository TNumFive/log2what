#ifndef FILE_WRITER_HPP
#define FILE_WRITER_HPP

#include "../base/common.hpp"
#include "../base/writer.hpp"
#include <fstream>
#include <map>
#include <mutex>

namespace log2what {
struct file_info {
    size_t writer_num;
    std::mutex file_mutex;
    std::ofstream out;
};

class file_writer : public writer {
  private:
    static std::mutex file_map_mutex;
    static std::map<string, file_info> file_map;
    string file_dir;
    string file_name;
    size_t file_size;
    size_t file_num;
    file_info *fip;
    bool open_log_file();

  public:
    static constexpr size_t KB = 1024;
    static constexpr size_t MB = 1024 * KB;
    file_writer(string file_name = "root", string file_dir = "./log/", size_t file_size = MB, size_t file_num = 50);
    ~file_writer();
    void write(level l, string module_name, string comment, string data) override;
};
} // namespace log2what

#endif