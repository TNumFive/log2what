#ifndef FILE_WRITER_HPP
#define FILE_WRITER_HPP

#include "../base/writer.hpp"

namespace log2what
{
    static constexpr size_t KB = 1024;
    static constexpr size_t MB = 1024 * KB;

    class file_writer : public writer
    {
    private:
        void *file_info_ptr;
        virtual bool open_log_file();

    public:
        file_writer(const string &file_name = "root", const string &file_dir = "./log/",
                    const size_t file_size = MB, const size_t file_num = 50,
                    const bool keep_alive = true);
        file_writer(const file_writer &other) = delete;
        file_writer(file_writer &&other) = delete;
        file_writer &operator=(const file_writer &other) = delete;
        file_writer &operator=(file_writer &&other) = delete;
        ~file_writer() override;
        void write(const level l, const string &module_name, const string &comment, const string &data) override;
    };
} // namespace log2what

#endif