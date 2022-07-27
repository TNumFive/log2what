#ifndef LOG2_HPP
#define LOG2_HPP

#include "./common.hpp"
#include "./writer.hpp"
#include <list>
#include <string>

namespace log2what {
/**
 * @brief base log class
 *
 */
class log2 {
  private:
    writer *writer_ptr;
    string module_name;

  public:
    /**
     * @brief Construct a new log2 object
     *
     * @param module_name
     * @param writer_ptr unique ptr as destructor will free it
     */
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

/**
 * @brief log2 with list of writers
 *
 */
class log2lots {
  private:
    std::list<writer *> writer_list;
    string module_name;
    void write(level l, string comment, string data) {
        for (auto &&i : writer_list) {
            i->write(l, module_name, comment, data);
        }
    }

  public:
    log2lots(string module_name = "root") : module_name{module_name} {};
    ~log2lots() {
        while (!writer_list.empty()) {
            delete writer_list.front();
            writer_list.pop_front();
        }
    }
    log2lots &append_writer(writer *w) {
        this->writer_list.push_back(w);
        return *this;
    }
    void trace(string comment = "", string data = "") {
        write(level::TRACE, comment, data);
    }
    void debug(string comment = "", string data = "") {
        write(level::DEBUG, comment, data);
    }
    void info(string comment = "", string data = "") {
        write(level::INFO, comment, data);
    }
    void warn(string comment = "", string data = "") {
        write(level::WARN, comment, data);
    }
    void error(string comment = "", string data = "") {
        write(level::ERROR, comment, data);
    }
};
} // namespace log2what
#endif