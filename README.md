# log2what
> 在开发过程中遇到了一些日志相关的问题，因而想要编写一个简单的日志系统，满足基本需求且易于拓展。
## 已实现功能
### 写入文件
提供了`file_writer`，可以设置日志文件夹、单个日志文件大小和单日志对象保留的总日志文件数。
### 写入数据库
提供了将日志内容写入数据库（sqlite3）的`db_writer`.
### 信号触发机制
提供了`buffered_shell`，会预先缓存一定数量的日志，当遇到指定等级的日志时便会一次性写出所有缓存的日志和当前日志以及未来一定条数的日志。
### 同时使用多种写入方式
提供了`log2lots`，一个能够同时调用多个writer进行日志写入功能的`logger`。
## 快速开始
> 引用使用到的功能的头文件，并在编译时加入涉及到的对应的代码文件即可；
```cpp
#include "../base/log2what.hpp"
#include "../base/shell.hpp"
#include "../buffered_shell/buffered_shell.hpp"
#include "../db_writer/db_writer.hpp"
#include "../file_writer/file_writer.hpp"
#include <iostream>

using namespace std;
using namespace log2what;

int main(int argc, char const *argv[])
{
    log2lots logger{};
    logger.append_writer(unique_ptr<shell>(
        new shell{log_level::INFO, unique_ptr<writer>(new writer)}));
    logger.append_writer(unique_ptr<db_writer>{new db_writer});
    logger.append_writer(unique_ptr<buffered_shell>(new buffered_shell{
        log_level::WARN, unique_ptr<file_writer>{new file_writer}}));
    logger.info("start testing");
    for (size_t i = 0; i < 10000; i++)
    {
        logger.debug(to_string(i), to_string(i));
    }
    logger.info("end test");
    return 0;
}
```
```bash
g++ -g  main.cpp ../db_writer/db_writer.cpp ../file_writer/file_writer.cpp -lsqlite3
```