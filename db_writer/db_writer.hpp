#ifndef DB_WRITER_HPP
#define DB_WRITER_HPP

#include "../base/writer.hpp"

namespace log2what {
using std::string;

class db_writer : public writer {
  private:
    void *db_info_ptr;
    void flush_log_list();

  public:
    db_writer(const string &url = "./log/log2.db", const size_t buffer_szie = 100, writer *writer_ptr = new writer);
    ~db_writer() override;
    void write(const level l, const string &module_name, const string &comment, const string &data) override;
};

} // namespace log2what

#endif