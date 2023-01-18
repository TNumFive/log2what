/**
 * @file log2what.hpp
 * @author TNumFive
 * @brief Logger class defined here.
 * @version 0.1
 * @date 2023-01-08
 *
 * @copyright Copyright (c) 2023
 *
 */

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
     * @brief Base class of logger.
     */
    class logger
    {
    public:
        using string = std::string;
        /**
         * @brief Construct a new logger object.
         *
         * @param module Name of module.
         */
        logger(string module = "root") { this->module == module; }
        /**
         * @brief Copy constructor deleted.
         *
         * @param other Other logger.
         */
        logger(const logger &other) = delete;
        /**
         * @brief Copy assign constructor deleted.
         *
         * @param other Other logger.
         * @return logger&
         */
        logger &operator=(const logger &other) = delete;
        /**
         * @brief Defaut Destructor.
         */
        virtual ~logger() = default;
        /**
         * @brief Use writer to write log.
         *
         * @param level Level of log.
         * @param comment Content of log.
         * @param data Data attached.
         */
        virtual void write(const log_level level, const string &comment,
                           const string &data) = 0;
        /**
         * @brief Write trace level log.
         *
         * @param comment Content of log.
         * @param data Data attached.
         */
        void trace(const string &comment = "", const string &data = "")
        {
            this->write(log_level::TRACE, comment, data);
        }
        /**
         * @brief Write debug level log.
         *
         * @param comment Content of log.
         * @param data Data attached.
         */
        void debug(const string &comment = "", const string &data = "")
        {
            this->write(log_level::DEBUG, comment, data);
        }
        /**
         * @brief Write info level log.
         *
         * @param comment Content of log.
         * @param data Data attached.
         */
        void info(const string &comment = "", const string &data = "")
        {
            this->write(log_level::INFO, comment, data);
        }
        /**
         * @brief Write warn level log.
         *
         * @param comment Content of log.
         * @param data Data attached.
         */
        void warn(const string &comment = "", const string &data = "")
        {
            this->write(log_level::WARN, comment, data);
        }
        /**
         * @brief Write error level log.
         *
         * @param comment Content of log.
         * @param data Data attached.
         */
        void error(const string &comment = "", const string &data = "")
        {
            this->write(log_level::ERROR, comment, data);
        }

    protected:
        string module;
    };

    /**
     * @brief Logger with one writer.
     */
    class log2one : public logger
    {
    public:
        using string = logger::string;
        using unique_ptr_writer = std::unique_ptr<writer>;
        /**
         * @brief Default constructor.
         *
         * @param module Name of module.
         * @param writer_unique_ptr Unique pointer of writer.
         */
        log2one(const string &module = "root",
                unique_ptr_writer &&writer_unique_ptr = unique_ptr_writer{
                    new writer})
        {
            this->module = module;
            this->writer_unique_ptr = std::move(writer_unique_ptr);
        }
        /**
         * @brief Copy constructor deleted.
         *
         * @param other Other logger.
         */
        log2one(const log2one &other) = delete;
        /**
         * @brief Copy assign constructor deleted.
         *
         * @param other Other logger.
         * @return log2one& Self.
         */
        log2one &operator=(const log2one &other) = delete;
        /**
         * @brief Move constructor.
         *
         * @param other Other logger.
         */
        log2one(log2one &&other) { this->swap(other); }
        /**
         * @brief Move assign constructor.
         *
         * @param other Other logger.
         * @return log2one& Self.
         */
        log2one &operator=(log2one &&other) { return this->swap(other); }
        /**
         * @brief Default destructor.
         */
        ~log2one() override = default;
        /**
         * @brief Use writer to write log.
         *
         * @param level Level of log.
         * @param comment Content of log.
         * @param data Data attached.
         */
        void write(const log_level level, const string &comment,
                   const string &data) override
        {
            this->writer_unique_ptr->write(level, this->module, comment, data);
        }

    protected:
        unique_ptr_writer writer_unique_ptr;
        /**
         * @brief Implementation of swap action.
         *
         * @param other Other logger.
         * @return log2one& Self.
         */
        log2one &swap(log2one &other)
        {
            if (this != &other)
            {
                std::swap(this->module, other.module);
                std::swap(this->writer_unique_ptr, other.writer_unique_ptr);
            }
            return *this;
        }
    };

    /**
     * @brief Logger with lots of writers.
     */
    class log2lots : public logger
    {
    public:
        using string = logger::string;
        using unique_ptr_writer = std::unique_ptr<writer>;
        /**
         * @brief Default onstructor.
         *
         * @param module Name of module.
         */
        log2lots(const string &module = "root") { this->module = module; }
        /**
         * @brief Copy constructor deleted.
         *
         * @param other Other logger.
         */
        log2lots(const log2lots &other) = delete;
        /**
         * @brief Copy assign constructor deleted.
         *
         * @param other Other logger.
         * @return log2lots& Self.
         */
        log2lots &operator=(const log2lots &other) = delete;
        /**
         * @brief Move constructor.
         *
         * @param other Other logger.
         */
        log2lots(log2lots &&other) { this->swap(other); }
        /**
         * @brief Move assign constructor.
         *
         * @param other Other logger.
         * @return log2lots& Self.
         */
        log2lots &operator=(log2lots &&other) { return this->swap(other); }
        /**
         * @brief Default destructor.
         */
        ~log2lots() override = default;
        /**
         * @brief Add writer to writer vector
         *
         * @param writer_unique_ptr new unique pointer of writer
         * @return log2lots Self.
         */
        virtual log2lots &append_writer(unique_ptr_writer &&writer_unique_ptr)
        {
            this->writer_unique_ptr_vector.push_back(
                std::move(writer_unique_ptr));
            return *this;
        }
        /**
         * @brief Use writer to write log.
         *
         * @param level Level of log.
         * @param comment Content of log.
         * @param data Data attached.
         */
        void write(const log_level level, const string &comment,
                   const string &data) override
        {
            for (auto &&writer_unique_ptr : this->writer_unique_ptr_vector)
            {
                writer_unique_ptr->write(level, this->module, comment, data);
            }
        }

    protected:
        std::vector<unique_ptr_writer> writer_unique_ptr_vector;
        /**
         * @brief Implementation of swap action.
         *
         * @param other Other logger.
         * @return log2lots& Self.
         */
        log2lots &swap(log2lots &other)
        {
            if (this != &other)
            {
                std::swap(this->module, other.module);
                std::swap(this->writer_unique_ptr_vector,
                          other.writer_unique_ptr_vector);
            }
            return *this;
        }
    };
} // namespace log2what
#endif