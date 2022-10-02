#ifndef LOG2WHAT_DB_WRITER_HPP
#define LOG2WHAT_DB_WRITER_HPP

#include "../base/writer.hpp"

namespace log2what
{
    using std::string;

    class db_writer : public writer
    {
    private:
        using string = std::string;
        void *db_info_ptr;
        void flush_log_list();

    public:
        db_writer(const string &url = "./log/log2.db",
                  const size_t buffer_szie = 100,
                  bool keep_alive = true,
                  std::unique_ptr<writer> &&writer_uptr = std::make_unique<writer>());
        db_writer(const db_writer &other) = delete;
        db_writer(db_writer &&other) = delete;
        db_writer &operator=(const db_writer &other) = delete;
        db_writer &operator=(db_writer &&other) = delete;
        ~db_writer() override;
        void write(const log_level level, const string &module_name,
                   const string &comment, const string &data,
                   const int64_t timestamp_nano) override;
    };

} // namespace log2what

#endif