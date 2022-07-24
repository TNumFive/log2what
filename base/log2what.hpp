#ifndef LOG2_HPP
#define LOG2_HPP

#include "./common.hpp"
#include "./writer.hpp"
#include <string>

namespace log2what {
using std::string;
class log2 {
  private:
    writer *writer_ptr;
    string module_name;

  public:
    log2(string module_name = "root", writer *writer_ptr = new writer) {
        this->module_name = module_name;
        this->writer_ptr = writer_ptr;
    }
    ~log2() {
        if (this->writer_ptr != nullptr) {
            delete this->writer_ptr;
            this->writer_ptr = nullptr;
        }
    }
    void trace(string comment = "", string data = "") {
        writer_ptr->write(level::TRACE, module_name, comment, data);
    }
    void debug(string comment = "", string data = "") {
        writer_ptr->write(level::DEBUG, module_name, comment, data);
    }
    void info(string comment = "", string data = "") {
        writer_ptr->write(level::INFO, module_name, comment, data);
    }
    void warn(string comment = "", string data = "") {
        writer_ptr->write(level::WARN, module_name, comment, data);
    }
    void error(string comment = "", string data = "") {
        writer_ptr->write(level::ERROR, module_name, comment, data);
    }
};
} // namespace log2what

#endif