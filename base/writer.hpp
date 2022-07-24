#ifndef WRITER_HPP
#define WRITER_HPP

#include "./common.hpp"
#include <iostream>
#include <string>

namespace log2what {
using std::string;

/**
 * @brief base write class
 *
 */
class writer {
  private:
    int zone;

  public:
    writer(int z) : zone{z} {};
    writer() {
        // try to obtain the time zone by ctime fun localtime
        int64_t t = get_timestamp<std::chrono::seconds>();
        std::lock_guard<std::mutex> lock{time_lock};
        std::tm gtp = *std::gmtime(&t);
        std::tm ltp = *std::localtime(&t);
        int64_t gt = std::mktime(&gtp);
        zone = (t - gt - (ltp.tm_isdst > 0 ? 3600 : 0)) / 3600;
    };
    ~writer() = default;
    virtual void write(level l, string module_name, string comment, string data) {
        constexpr int SEC_TO_NANO = 1000000000;
        int64_t nano_timestamp = get_nano_timestamp();
        std::cout << get_datetime(nano_timestamp / SEC_TO_NANO,
                                  nano_timestamp % SEC_TO_NANO, zone)
                  << " " << to_string(l)
                  << " " << module_name
                  << " |%| " << comment
                  << " |%| " << data << std::endl;
    }
};

} // namespace log2what
#endif