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
    log2(const string &module_name = "root", writer *writer_ptr = new writer);
};

// log2lots，能够同时绑定多个writer，将一条log信息同时传递给它们
class log2lots{
    log2lots(const string &module_name = "root");
    // 增加一个writer
    virtual log2lots &append_writer(writer *w);
};
```
- `writer`
```cpp
// 最基础的writer，将log信息直接通过std::cout打印
class writer{
    writer() = default;
};
// file writer, 将log写入文件，可以设置保留文件个数，文件大小
// 假定不会在1ms内写入超过file_szie/log_fixed_min_length的log
// 文件名将采用创建时的毫秒时间戳如root.log.20220814_105832_204
class file_writer : public writer {
    file_writer(const string &file_name = "root", const string &file_dir = "./log/", const size_t file_size = MB, const size_t file_num = 50);
};
// database writer, 将log写入指定数据库
// 默认使用的是sqlite3，需要安装对应的开发环境
class db_writer : public writer {
    db_writer(const string &url = "./log/log2.db", const size_t buffer_szie = 100, writer *writer_ptr = new writer);
};
```
- `shell`
```cpp
// 继承于writer，接受一个writer指针，对writer收到的log信息做过滤，默认过滤info以下级别的log
class shell : public writer {
    shell(writer *writer_ptr = new writer);
};
```
### 使用方式
引用涉及到的模块，并在编译时加入对应的`cpp`文件即可  
可以参考**测试**中的`test_and_script/test_and_bench.cpp`和`build.sh`
```cpp
// 初始化一个向文件写入log的简单logger
auto sim_logger = log2{"root", new file_writer};
// 初始化一个，
// 控制台输出info及以上level的log、
// 将全量log写入文件的、
// 将log信息记录到数据库的
// logger
auto multi_logger = log2lots()
    .append_writer(new shell{new writer})
    .append_writer(new file_writer)
    .append_writer(new db_writer);
```
### 测试与速度
`test_and_script/test_and_bench.cpp`记录了一些用来测试`writer`可用性和速度测试的函数，可以使用下述脚本编译测试
```bash
# put this script under ${repo}/build/
mkdir -p ./temp/
g++ -g -c ../file_writer/file_writer.cpp -o ./temp/file_writer.o &&
g++ -g -c ../db_writer/db_writer.cpp -o ./temp/db_writer.o &&
g++ -g -c ../test_and_bench/test_and_bench.cpp -o ./temp/main.o &&
g++ -g ./temp/main.o ./temp/file_writer.o ./temp/db_writer.o -l sqlite3
```