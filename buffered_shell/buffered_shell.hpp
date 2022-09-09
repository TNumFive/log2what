#ifndef BUFFERED_SHELL_HPP
#define BUFFERED_SHELL_HPP
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
            if (level >= mask)
            {
                // triggered
                if (left_to_write <= 0)
                {
                    // new trigger
                    writer_uptr->write(level, "buffered_shell", "triggered", "");
                }
                for (auto &&l : log_list)
                {
                    writer_uptr->write(l.level, l.module_name,
                                       l.comment, l.data, l.timestamp_nano);
                }
                writer_uptr->write(level, module_name, comment, data);
                lock_guard lock{buffer_mutex};
                log_list.clear();
                left_to_write = after;
                return;
            }
            if (left_to_write > 0)
            {
                writer_uptr->write(level, module_name, comment, data);
                lock_guard lock{buffer_mutex};
                left_to_write--;
                if (left_to_write == 0)
                {
                    writer_uptr->write(level, "buffered_shell", "output over", "");
                }
                return;
            }
            lock_guard lock{buffer_mutex};
            log_list.emplace_back(get_nano_timestamp(), level, module_name, comment, data);
            if (log_list.size() > before)
            {
                log_list.pop_front();
            }
        }
    };
} // namespace log2what
#endif