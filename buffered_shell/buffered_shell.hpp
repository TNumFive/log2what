#ifndef LOG2WHAT_BUFFERED_SHELL_HPP
#define LOG2WHAT_BUFFERED_SHELL_HPP
#include "../base/writer.hpp"
#include <list>
#include <memory>
#include <mutex>
namespace log2what
{
    /**
     * @brief instead of masking but triggering
     *
     */
    class buffered_shell : public writer
    {
    private:
        using string = std::string;
        using up_writer = std::unique_ptr<writer>;
        using lock_guard = std::lock_guard<std::mutex>;
        using seconds = std::chrono::seconds;
        up_writer writer_uptr;
        log_level mask = log_level::INFO;
        size_t before;
        size_t after;
        int64_t left_to_write;
        std::list<log> log_list;
        std::mutex buffer_mutex;

    public:
        buffered_shell(up_writer &&writer_uptr = std::make_unique<writer>(),
                       const size_t before = 100, const size_t after = 10)
        {
            this->writer_uptr = std::move(writer_uptr);
            this->before = before;
            this->after = after;
            this->left_to_write = 0;
        }
        buffered_shell(const log_level mask,
                       up_writer &&writer_uptr = std::make_unique<writer>(),
                       const size_t before = 100, const size_t after = 10)
        {
            this->mask = mask;
            this->writer_uptr = std::move(writer_uptr);
            this->before = before;
            this->after = after;
            this->left_to_write = 0;
        }
        buffered_shell(const buffered_shell &other) = delete;
        buffered_shell(buffered_shell &&other) = delete;
        buffered_shell &operator=(const buffered_shell &other) = delete;
        buffered_shell &operator=(buffered_shell &&other) = delete;
        ~buffered_shell() override {}
        void write(const log_level level, const string &module_name,
                   const string &comment, const string &data,
                   const int64_t) override
        {
            lock_guard lock{buffer_mutex};
            if (level >= mask)
            {
                // triggered
                if (left_to_write <= 0)
                {
                    // new trigger
                    writer_uptr->write(
                        level, "buffered_shell", "begin",
                        get_localtime_str(get_timestamp<seconds>()),
                        log_list.front().timestamp_nano);
                }
                for (auto &&l : log_list)
                {
                    writer_uptr->write(l.level, l.module_name,
                                       l.comment, l.data, l.timestamp_nano);
                }
                writer_uptr->write(level, module_name, comment, data);
                log_list.clear();
                left_to_write = after;
                return;
            }
            if (left_to_write > 0)
            {
                writer_uptr->write(level, module_name, comment, data);
                left_to_write--;
                if (left_to_write == 0)
                {
                    writer_uptr->write(level, "buffered_shell", "ended",
                                       get_localtime_str(get_timestamp<seconds>()));
                }
                return;
            }
            log_list.emplace_back(get_nano_timestamp(), level, module_name, comment, data);
            if (log_list.size() > before)
            {
                log_list.pop_front();
            }
        }
    };
} // namespace log2what
#endif