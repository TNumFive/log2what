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
    using level_t = std::underlying_type_t<level>;

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
    ~shell() {
        if (writer_ptr != nullptr) {
            delete writer_ptr;
        }
    };
    virtual void write(level l, string module_name, string comment, string data) {
        if (l >= mask) {
            writer::write(l, module_name, comment, data);
        }
    }
};
} // namespace log2what
#endif