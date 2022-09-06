#ifndef LOG2_HPP
#define LOG2_HPP

#include "./common.hpp"
#include "./writer.hpp"
#include <list>
#include <string>

namespace log2what
{
    /**
     * @brief base log class
     *
     */
    class log2
    {
    private:
        writer *writer_ptr;
        string module_name;

    public:
        /**
         * @brief Construct a new log2 object
         *
         * @param module_name
         * @param writer_ptr unique ptr as destructor will free it
         */
        log2(const string &module_name = "root", writer *writer_ptr = new writer)
        {
            this->module_name = module_name;
            this->writer_ptr = writer_ptr;
        }
        log2(const log2 &other) = delete;
        log2(log2 &&other) = delete;
        log2 &operator=(const log2 &other) = delete;
        log2 &operator=(log2 &&other) = delete;
        virtual ~log2()
        {
            if (this->writer_ptr != nullptr)
            {
                delete this->writer_ptr;
                this->writer_ptr = nullptr;
            }
        }
        // virtual log2 *clone() { return new log2{}; };
        // virtual void swap(log2 &){};
        virtual void trace(const string &comment = "", const string &data = "")
        {
            writer_ptr->write(level::TRACE, module_name, comment, data);
        }
        virtual void debug(const string &comment = "", const string &data = "")
        {
            writer_ptr->write(level::DEBUG, module_name, comment, data);
        }
        virtual void info(const string &comment = "", const string &data = "")
        {
            writer_ptr->write(level::INFO, module_name, comment, data);
        }
        virtual void warn(const string &comment = "", const string &data = "")
        {
            writer_ptr->write(level::WARN, module_name, comment, data);
        }
        virtual void error(const string &comment = "", const string &data = "")
        {
            writer_ptr->write(level::ERROR, module_name, comment, data);
        }
    };

    /**
     * @brief log2 with list of writers
     *
     */
    class log2lots
    {
    private:
        std::list<writer *> writer_list;
        string module_name;
        virtual void write(const level l, const string &comment, const string &data)
        {
            for (auto &&i : writer_list)
            {
                i->write(l, module_name, comment, data);
            }
        }

    public:
        log2lots(const string &module_name = "root") : module_name{module_name} {};
        log2lots(const log2lots &other) = delete;
        log2lots(log2lots &&other) = delete;
        log2lots &operator=(const log2lots &other) = delete;
        log2lots &operator=(log2lots &&other) = delete;
        virtual ~log2lots()
        {
            while (!writer_list.empty())
            {
                delete writer_list.front();
                writer_list.pop_front();
            }
        }
        virtual log2lots &append_writer(writer *w)
        {
            this->writer_list.push_back(w);
            return *this;
        }
        virtual void trace(const string &comment = "", const string &data = "")
        {
            write(level::TRACE, comment, data);
        }
        virtual void debug(const string &comment = "", const string &data = "")
        {
            write(level::DEBUG, comment, data);
        }
        virtual void info(const string &comment = "", const string &data = "")
        {
            write(level::INFO, comment, data);
        }
        virtual void warn(const string &comment = "", const string &data = "")
        {
            write(level::WARN, comment, data);
        }
        virtual void error(const string &comment = "", const string &data = "")
        {
            write(level::ERROR, comment, data);
        }
    };
} // namespace log2what
#endif