/**
 * @file buffered_shell.hpp
 * @author TNumFive
 * @brief Shell that will buffer logs and write when triggered.
 * @version 0.1
 * @date 2023-01-13
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef LOG2WHAT_BUFFERED_SHELL_HPP
#define LOG2WHAT_BUFFERED_SHELL_HPP
#include "../base/writer.hpp"
#include <list>
#include <memory>
#include <mutex>
namespace log2what
{
    /**
     * @brief Shell that will write buffered log when triggered.
     *
     */
    class buffered_shell : public writer
    {
    public:
        using string = std::string;
        using unique_ptr_writer = std::unique_ptr<writer>;
        using lock_guard = std::lock_guard<std::mutex>;
        /**
         * @brief Construct a new buffered shell object.
         *
         * @param mask Which level that will trigger log writting.
         * @param writer_unique_ptr Writer pointer held.
         * @param before How many logs to be buffered.
         * @param after How many logs to write after triggered.
         */
        buffered_shell(const log_level mask = log_level::INFO,
                       unique_ptr_writer &&writer_unique_ptr =
                           unique_ptr_writer{new writer},
                       const size_t before = 100, const size_t after = 10)
        {
            this->mask = mask;
            this->writer_unique_ptr = std::move(writer_unique_ptr);
            this->before = before;
            this->after = after;
            this->left_to_write = 0;
        }
        /**
         * @brief Copy constructor deleted.
         *
         * @param other Other shell.
         */
        buffered_shell(const buffered_shell &other) = delete;
        /**
         * @brief Copy assign constructor.
         *
         * @param other Other shell.
         * @return buffered_shell& Self.
         */
        buffered_shell &operator=(const buffered_shell &other) = delete;
        /**
         * @brief Move constructor deleted.
         *
         * @param other Other shell.
         */
        buffered_shell(buffered_shell &&other) = delete;
        /**
         * @brief Move assign constructor deleted.
         *
         * @param other Other shell.
         * @return buffered_shell& Self.
         */
        buffered_shell &operator=(buffered_shell &&other) = delete;
        /**
         * @brief Default destructor.
         *
         */
        ~buffered_shell() override = default;
        /**
         * @brief Buffer logs until triggerd.
         *
         * @param level Log level.
         * @param module Module name.
         * @param comment Content of log.
         * @param data Data attached.
         */
        void write(const log_level level, const string &module,
                   const string &comment, const string &data,
                   const int64_t) override
        {
            lock_guard lock{buffer_mutex};
            if (level >= this->mask)
            {
                // triggered
                if (this->left_to_write == 0)
                {
                    // new trigger
                    this->writer_unique_ptr->write(
                        level, "buffered_shell", "begin", "",
                        log_list.front().timestamp_nano);
                }
                for (auto &&i : log_list)
                {
                    this->writer_unique_ptr->write(i.level, i.module, i.comment,
                                                   i.data, i.timestamp_nano);
                }
                this->writer_unique_ptr->write(level, module, comment, data);
                this->log_list.clear();
                this->left_to_write = this->after;
                return;
            }
            if (this->left_to_write > 0)
            {
                this->writer_unique_ptr->write(level, module, comment, data);
                this->left_to_write--;
                if (this->left_to_write == 0)
                {
                    this->writer_unique_ptr->write(level, "buffered_shell",
                                                   "ended", "");
                }
                return;
            }
            this->log_list.emplace_back(get_nano_timestamp(), level, module,
                                        comment, data);
            if (this->log_list.size() > this->before)
            {
                this->log_list.pop_front();
            }
        }

    private:
        unique_ptr_writer writer_unique_ptr;
        log_level mask;
        size_t before;
        size_t after;
        size_t left_to_write;
        std::list<log> log_list;
        std::mutex buffer_mutex;
    };
} // namespace log2what
#endif