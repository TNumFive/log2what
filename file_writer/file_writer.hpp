#ifndef FILE_WRITER_HPP
#define FILE_WRITER_HPP

#include "../base/writer.hpp"

namespace log2what {
static constexpr size_t KB = 1024;
static constexpr size_t MB = 1024 * KB;

class file_writer : public writer {
  private:
    void *file_info_ptr;
    virtual bool open_log_file();
    virtual void clean_log_file();

  public:
    file_writer(string file_name = "root", string file_dir = "./log/", size_t file_size = MB, size_t file_num = 50);
    ~file_writer() override;
    void write(level l, string module_name, string comment, string data) override;
};
} // namespace log2what

#endif