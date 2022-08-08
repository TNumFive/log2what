#ifndef DB_WRITER_HPP
#define DB_WRITER_HPP

#include "../base/writer.hpp"

namespace log2what {
using std::string;

class db_writer : public writer {
  private:
    string url;
    void *db_info_ptr;
    void flush_log_list();

  public:
    db_writer(string url = "./log/log2.db", size_t buffer_szie = 100, writer *writer_ptr = new writer);
    ~db_writer() override;
    void write(level l, string module_name, string comment, string data) override;
};

} // namespace log2what

#endif