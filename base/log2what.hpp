#ifndef LOG2WHAT_LOG2_HPP
#define LOG2WHAT_LOG2_HPP

#include "./common.hpp"
#include "./writer.hpp"
#include <memory>
#include <string>
#include <vector>

namespace log2what
{
    /**
     * @brief base log class
     *
     */
    class log2
    {
    private:
        using string = std::string;
        using up_writer = std::unique_ptr<writer>;
        up_writer writer_uptr;
        string module_name;

    public:
        /**
         * @brief Construct a new log2 object
         *
         * @param module_name
         * @param writer_uptr unique_ptr as destructor will free it
         */
        log2(const string &module_name = "root",
             up_writer &&writer_uptr = std::make_unique<writer>())
        {
            this->module_name = module_name;
            this->writer_uptr = std::move(writer_uptr);
        }
        log2(const log2 &other) = delete;
        log2(log2 &&other)
        {
            this->swap(std::move(other));
        }
        log2 &operator=(const log2 &other) = delete;
        log2 &operator=(log2 &&other)
        {
            this->swap(std::move(other));
            return *this;
        }
        virtual ~log2() {}
        virtual void swap(log2 &&other)
        {
            if (this != &other)
            {
                module_name = std::move(other.module_name);
                writer_uptr = std::move(other.writer_uptr);
            }
        }
        virtual void write(const log_level level,
                           const string &comment, const string &data)
        {
            if (writer_uptr)
            {
                writer_uptr->write(level, module_name, comment, data);
            }
        }
        virtual void trace(const string &comment = "", const string &data = "")
        {
            write(log_level::TRACE, comment, data);
        }
        virtual void debug(const string &comment = "", const string &data = "")
        {
            write(log_level::DEBUG, comment, data);
        }
        virtual void info(const string &comment = "", const string &data = "")
        {
            write(log_level::INFO, comment, data);
        }
        virtual void warn(const string &comment = "", const string &data = "")
        {
            write(log_level::WARN, comment, data);
        }
        virtual void error(const string &comment = "", const string &data = "")
        {
            write(log_level::ERROR, comment, data);
        }
    };

    /**
     * @brief log2 with list of writers
     *
     */
    class log2lots
    {
    private:
        using string = std::string;
        using up_writer = std::unique_ptr<writer>;
        std::vector<up_writer> writer_uptr_vector;
        string module_name;

    public:
        log2lots(const string &module_name = "root")
        {
            this->module_name = module_name;
        }
        log2lots(const log2lots &other) = delete;
        log2lots &operator=(const log2lots &other) = delete;
        log2lots(log2lots &&other)
        {
            this->swap(std::move(other));
        }
        log2lots &operator=(log2lots &&other)
        {
            this->swap(std::move(other));
            return *this;
        }
        virtual void swap(log2lots &&other)
        {
            if (this != &other)
            {
                module_name = std::move(other.module_name);
                writer_uptr_vector = std::move(other.writer_uptr_vector);
            }
        }
        virtual ~log2lots() {}
        virtual log2lots &append_writer(up_writer &&writer_uptr)
        {
            this->writer_uptr_vector.push_back(std::move(writer_uptr));
            return *this;
        }
        virtual void write(const log_level level,
                           const string &comment, const string &data)
        {
            for (auto &&up : writer_uptr_vector)
            {
                up->write(level, module_name, comment, data);
            }
        }
        virtual void trace(const string &comment = "", const string &data = "")
        {
            write(log_level::TRACE, comment, data);
        }
        virtual void debug(const string &comment = "", const string &data = "")
        {
            write(log_level::DEBUG, comment, data);
        }
        virtual void info(const string &comment = "", const string &data = "")
        {
            write(log_level::INFO, comment, data);
        }
        virtual void warn(const string &comment = "", const string &data = "")
        {
            write(log_level::WARN, comment, data);
        }
        virtual void error(const string &comment = "", const string &data = "")
        {
            write(log_level::ERROR, comment, data);
        }
    };
} // namespace log2what
#endif