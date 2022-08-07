#ifndef DB_WRITER_HPP
#define DB_WRITER_HPP

#include "../base/common.hpp"
#include "../base/log2what.hpp"
#include "../base/writer.hpp"
#include <mutex>

namespace log2what {
using std::string;

class db_writer : public writer {
  private:
    string url;
    void *db_handle;
    void *stmt_handle;
    std::mutex *db_mutex;
    log2 *logger_ptr;

  public:
    db_writer(string url = "./log/log2.db", writer *writer_ptr = new writer);
    ~db_writer() override;
    void write(level l, string module_name, string comment, string data) override;
};

} // namespace log2what

#endif