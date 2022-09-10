# log2what
### 项目目标
1. 简单的c++日志记录器实现
2. 尽量不使用复杂的工具或概念
3. 为后续拓展留出空间
4. 可以接受的写入速度
### 代码结构
- `logger`
```cpp
// log2，最基础的logger，绑定一个writer用来写log
class log2{
    log2(const string &module_name = "root",
             up_writer &&writer_uptr = std::make_unique<writer>());
};

// log2lots，能够同时绑定多个writer，将一条log信息传递给它们
class log2lots{
    log2lots(const string &module_name = "root");
    // 增加一个writer
    virtual log2lots &append_writer(up_writer &&writer_uptr);
};
```
- `writer`
```cpp
// 最基础的writer，将log信息直接通过std::cout打印
class writer{
    writer() = default;
};

// file writer, 将log写入文件，可以设置保留文件个数，文件大小
// 假定不会在1ms内写入超过file_szie/avg(log_length)的log
// 文件名将采用创建时的毫秒时间戳如root.log.20220814_105832_204
class file_writer : public writer {
    file_writer(const string &file_name = "root", const string &file_dir = "./log/",
                const size_t file_size = MB, const size_t file_num = 50,
                const bool keep_alive = true);
};

// database writer, 将log写入指定数据库
// 默认使用的是sqlite3，需要安装对应的开发环境
class db_writer : public writer {
    db_writer(const string &url = "./log/log2.db",
              const size_t buffer_szie = 100,
              bool keep_alive = true,
              std::unique_ptr<writer> &&writer_uptr = std::make_unique<writer>());
};
```
- `shell`
```cpp
// 继承于writer，接受一个writer指针，对writer收到的log信息做过滤，默认过滤info以下级别的log
class shell : public writer {
    shell(writer *writer_ptr = new writer);
};

/**
 *  继承于writer，接受一个writer指针，缓存writer接收到的消息
 *  当接收到指定level的log时，输出缓存的log和当前log以及后续一定条数的log
 */
class buffered_shell : public writer {
    buffered_shell(up_writer &&writer_uptr = std::make_unique<writer>(),
                       const size_t before = 100, const size_t after = 10)
};
```
### 使用方式
引用涉及到的模块，并在编译时加入对应的`cpp`文件即可  
```cpp
log2lots get_logger(string module_name = __FILE__)
{
    log2lots logger{module_name};
    // log info to std
    logger.append_writer(make_unique<shell>());
    // buffered log until meets warn
    logger.append_writer(make_unique<buffered_shell>(
        log_level::WARN, make_unique<file_writer>(module_name), 50, 10));
    // write all log to file
    logger.append_writer(make_unique<file_writer>(module_name, "./log_full/"));
    // write all log to db and 
    logger.append_writer(make_unique<db_writer>(
        "./log/log2db", 100, true, make_unique<file_writer>("log2db")));
    return logger;
}
```
### 测试与速度
`test_and_script/test_and_bench.cpp`记录了一些用来测试`writer`可用性和速度测试的函数，可以使用下述脚本编译测试
```bash
# put this script under ${repo}/build/
mkdir -p ./temp/
g++ -g -c ../file_writer/file_writer.cpp -o ./temp/file_writer.o &&
g++ -g -c ../db_writer/db_writer.cpp -o ./temp/db_writer.o &&
g++ -g -c ../test_and_bench/test_and_bench.cpp -o ./temp/main.o &&
g++ -g ./temp/main.o ./temp/file_writer.o ./temp/db_writer.o -l sqlite3 -lpthread
```