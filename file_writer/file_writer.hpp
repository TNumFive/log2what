#ifndef LOG2WHAT_FILE_WRITER_HPP
#define LOG2WHAT_FILE_WRITER_HPP

#include "../base/writer.hpp"

namespace log2what
{
    static constexpr size_t KB = 1024;
    static constexpr size_t MB = 1024 * KB;

    class file_writer : public writer
    {
    private:
        using string = std::string;
        void *file_info_ptr;
        bool open_log_file();

    public:
        file_writer(const string &file_name = "root", const string &file_dir = "./log/",
                    const size_t file_size = MB, const size_t file_num = 50,
                    const bool keep_alive = true);
        file_writer(const file_writer &other) = delete;
        file_writer(file_writer &&other) = delete;
        file_writer &operator=(const file_writer &other) = delete;
        file_writer &operator=(file_writer &&other) = delete;
        ~file_writer() override;
        void write(const log_level level, const string &module_name,
                   const string &comment, const string &data,
                   const int64_t timestamp_nano = 0) override;
    };
} // namespace log2what

#endif