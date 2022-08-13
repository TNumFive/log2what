#include "../base/log2what.hpp"
#include "../base/shell.hpp"
#include "../file_writer/file_writer.hpp"
#include <dirent.h>
#include <fstream>
#include <list>
#include <map>
#include <mutex>
#include <random>
#include <regex>
#include <string>
#include <thread>
#include <vector>

using log2what::get_nano_timestamp;
using log2std = log2what::log2;
using log2what::mkdir;
using writer = log2what::writer;
using file_writer = log2what::file_writer;
using log2what::level;
using log2what::shell;
using std::fstream;
using std::list;
using std::map;
using std::regex;
using std::regex_match;
using std::string;
using std::thread;
using std::to_string;

static log2std logger{"benchmark", new shell{level::INFO, new writer}};

void sync_cout(string comment, string data) {
    static std::mutex cout_mutex;
    std::lock_guard<std::mutex> lock{cout_mutex};
    logger.debug(comment, data);
}

void do_nothing() {}

void benchmark_thread() {
    logger.info("start test", "start 1000 thread");
    auto start = get_nano_timestamp();
    for (size_t i = 0; i < 1000; i++) {
        thread t{do_nothing};
        t.join();
    }
    auto end = get_nano_timestamp();
    auto time_consumed = (end - start);
    // about 0.04ms per thread
    logger.info("end test", to_string(time_consumed / 1e6) + "ms");
}

/**
 * @brief try to mimic the ls cmd
 *
 * @param path
 * @return std::list<string>
 */
inline list<string> ls(string path) {
    DIR *dir;
    dirent64 *diread;
    dir = opendir(path.c_str());
    if (dir != nullptr) {
        list<string> file_list;
        diread = readdir64(dir);
        while (diread != nullptr) {
            file_list.push_back(diread->d_name);
            diread = readdir64(dir);
        }
        closedir(dir);
        file_list.sort();
        return file_list;
    }
    return {};
}

void benchmark_mkdir() {
    mkdir("./benchmark/");
    logger.info("start test", "open 1000 dirs");
    auto start = get_nano_timestamp();
    for (size_t i = 0; i < 1000; i++) {
        mkdir("./benchmark/" + to_string(i));
    }
    auto end = get_nano_timestamp();
    auto time_consumed = (end - start);
    // about 1.5ms one dir
    logger.info("end test", to_string(time_consumed / 1e6) + "ms");

    logger.info("start test", "read dir with 1000-files 1000 times");
    start = get_nano_timestamp();
    for (size_t i = 0; i < 1000; i++) {
        ls("./benchmark/");
    }
    end = get_nano_timestamp();
    time_consumed = (end - start);
    // about 1.035ms one dir
    logger.info("end test", to_string(time_consumed / 1e6) + "ms");
}

void benchmark_lock() {
    std::mutex test;
    logger.info("start test", "lock and unlock 1000 times");
    auto start = get_nano_timestamp();
    for (size_t i = 0; i < 1000; i++) {
        std::unique_lock<std::mutex> lock{test};
    }
    auto end = get_nano_timestamp();
    auto time_consumed = (end - start);
    // about 0.000035 each time
    logger.info("end test", to_string(time_consumed / 1e6) + "ms");
}

void benchmark_file() {
    mkdir("./benchmark/");
    map<string, fstream> file_map;
    for (size_t i = 0; i < 1000; i++) {
        std::stringstream ss;
        ss << "./benchmark/" << to_string(i) << ".log";
        file_map[ss.str()];
    }
    logger.info("start test", "open 1000 files");
    auto start = get_nano_timestamp();
    for (auto &&i : file_map) {
        i.second.open(i.first, std::ios::app);
    }
    auto end = get_nano_timestamp();
    auto time_consumed = (end - start);
    // about 0.015ms per file
    logger.info("end test", to_string(time_consumed / 1e6) + "ms");

    logger.info("start test", "close 1000 files");
    start = get_nano_timestamp();
    for (auto &&i : file_map) {
        i.second.close();
    }
    end = get_nano_timestamp();
    time_consumed = (end - start);
    // about 0.005ms per file
    logger.info("end test", to_string(time_consumed / 1e6) + "ms");

    logger.info("start test", "remove 1000 files");
    start = get_nano_timestamp();
    for (auto &&i : file_map) {
        std::remove(i.first.c_str());
    }
    end = get_nano_timestamp();
    time_consumed = (end - start);
    // about 0.01ms per file
    logger.info("end test", to_string(time_consumed / 1e6) + "ms");
}

int random_int(int start = 1, int end = 5) {
    std::random_device r;
    std::default_random_engine engine(r());
    std::uniform_int_distribution<int> uniform_distribution{start, end};
    return uniform_distribution(engine);
}

void random_log_writer(string name, int log_num) {
    while (log_num > 0) {
        // set up random nums of logger
        std::vector<log2std *> logger_vector;
        int logger_num = random_int(1, 5);
        sync_cout(name, string().append("logger_num:").append(to_string(logger_num)));
        // initializing them
        for (size_t i = 0; i < logger_num; i++) {
            logger_vector.push_back(
                new log2std{name + "_" + to_string(i),
                            new file_writer{name, "./log/", log2what::MB, 50}});
        }
        // write random nums of log
        int cycle_to_write = random_int(1, 100);
        cycle_to_write = log_num > cycle_to_write ? cycle_to_write : log_num;
        sync_cout(name, string().append("cycle_to_write:").append(to_string(cycle_to_write)).append(" log_num left:").append(to_string(log_num)));
        // start loging
        for (size_t i = 0; i < cycle_to_write; i++) {
            logger_vector[random_int(1, logger_num) - 1]->info("random write test", to_string(i) + "_" + to_string(log_num));
            sync_cout(name, to_string(log_num));
            log_num--;
        }
        // release those logger
        for (size_t i = 0; i < logger_num; i++) {
            delete logger_vector[i];
        }
    }
}

int main(int argc, char const *argv[]) {
    logger.info("start mutlti thread writer test", "");
    auto start = get_nano_timestamp();
    thread t1{random_log_writer, "thread1", 250000};
    thread t2{random_log_writer, "thread2", 250000};
    thread t3{random_log_writer, "thread3", 250000};
    thread t4{random_log_writer, "thread4", 250000};
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    auto end = get_nano_timestamp();
    logger.info("end test", to_string((end - start) / 1e6) + "ms");
    return 0;
}
