#ifndef SHELL_HPP
#define SHELL_HPP

#include "./common.hpp"
#include "./writer.hpp"

namespace log2what {
/**
 * @brief mask log by rules
 *
 */
class shell : public writer {
  private:
    writer *writer_ptr;
    level mask = level::INFO;

  public:
    shell(writer *writer_ptr = new writer) {
        this->writer_ptr = writer_ptr;
    };
    shell(level mask, writer *writer_ptr = new writer) {
        this->mask = mask;
        this->writer_ptr = writer_ptr;
    }
    ~shell() override {
        if (writer_ptr != nullptr) {
            delete writer_ptr;
        }
    };
    void write(level l, string module_name, string comment, string data) override {
        if (l >= mask) {
            writer_ptr->write(l, module_name, comment, data);
        }
    }
};
} // namespace log2what
#endif