#ifndef LOG2WHAT_SHELL_HPP
#define LOG2WHAT_SHELL_HPP

#include "./common.hpp"
#include "./writer.hpp"
#include <memory>

namespace log2what
{
    /**
     * @brief mask log by rules
     *
     */
    class shell : public writer
    {
    private:
        using string = std::string;
        using up_writer = std::unique_ptr<writer>;
        up_writer writer_uptr;
        log_level mask = log_level::INFO;

    public:
        shell(up_writer &&writer_uptr = std::make_unique<writer>())
        {
            this->writer_uptr = std::move(writer_uptr);
        };
        shell(const log_level mask, up_writer &&writer_uptr = std::make_unique<writer>())
        {
            this->mask = mask;
            this->writer_uptr = std::move(writer_uptr);
        }
        shell(const shell &other) = delete;
        shell(shell &&other) = delete;
        shell &operator=(const shell &other) = delete;
        shell &operator=(shell &&other) = delete;
        ~shell() override{};
        void write(const log_level l, const string &module_name,
                   const string &comment, const string &data,
                   const int64_t timestamp_nano) override
        {
            if (l >= mask)
            {
                writer_uptr->write(l, module_name, comment, data);
            }
        }
    };
} // namespace log2what
#endif