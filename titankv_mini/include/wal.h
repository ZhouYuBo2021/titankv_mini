#ifndef WAL_H
#define WAL_H

#include <fstream>
#include <mutex>
#include <string>
#include <vector>

// 前向声明
class KVStore;

class WAL {
public:
    WAL(const std::string& path);
    ~WAL();

    // 记录SET到操作日志
    void log_set(const std::string& key, const std::string& value);

    // 记录DEL到操作日志
    void log_del(const std::string& key);

    // 重放日志以恢复数据
    void replay(KVStore& store);

    // 获取日志文件路径
    const std::string& get_log_path() const {return log_path;}


private:
    std::string log_path;      // 日志文件路径
    std::ofstream log_file;    // 文件输出流
    std::mutex log_mutex;      // 互斥锁（保证读写日志的线程安全）
    size_t last_flush_size;    // 上次刷新时的文件大小

    // 禁止拷贝构造和赋值
    WAL(const WAL&) = delete;
    WAL& operator=(const WAL&) = delete;
};


#endif // WAL_H