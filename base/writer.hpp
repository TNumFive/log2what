#ifndef WRITER_HPP
#define WRITER_HPP

#include "./common.hpp"
#include <iostream>
#include <string>

namespace log2what {
using std::string;

class writer {
  private:
    /* data */
  public:
    writer(/* args */) = default;
    ~writer() = default;
    virtual void write(level l, string module_name, string comment, string data) {
        constexpr int SEC_TO_NANO = 1000000000;
        int64_t nano_timestamp = get_nano_timestamp();
        std::cout << get_datetime(nano_timestamp / SEC_TO_NANO,
                                  nano_timestamp % SEC_TO_NANO, 8)
                  << " " << to_string(l)
                  << " " << module_name
                  << " |%| " << comment
                  << " |%| " << data << std::endl;
    }
};

} // namespace log2what
#endif