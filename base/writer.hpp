#ifndef WRITER_HPP
#define WRITER_HPP

#include "./common.hpp"
#include <iostream>
#include <string>

namespace log2what {
/**
 * @brief base writer class
 *
 */
class writer {
  private:
  public:
    writer() = default;
    ~writer() = default;
    virtual void write(level l, string module_name, string comment, string data) {
        constexpr int SEC_TO_NANO = 1000000000;
        constexpr int MILL_TO_NANO = 1000000;
        int64_t nano_timestamp = get_nano_timestamp();
        int64_t sec_stamp = nano_timestamp / SEC_TO_NANO;
        int64_t precision = nano_timestamp % SEC_TO_NANO;
        precision /= MILL_TO_NANO;
        std::lock_guard<std::mutex> lock{time_lock};
        static char buffer[20];
        std::tm *ltp = std::localtime(&sec_stamp);
        std::strftime(buffer, sizeof(buffer), "%F %T", ltp);
        std::cout << buffer
                  << "." << std::setw(3) << std::setfill('0') << precision
                  << " " << to_string(l)
                  << " " << module_name
                  << " |%| " << comment
                  << " |%| " << data << std::endl;
    }
};
} // namespace log2what
#endif