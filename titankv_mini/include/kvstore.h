#ifndef KVSTORE_H
#define KVSTORE_H

#include <unordered_map>
#include <string>
#include <mutex>
#include <chrono>
#include <atomic>
#include <vector>

// 添加Wal类的前置声明
class WAL;

class KVStore{
public:
    // 构造函数
    KVStore(const std::string& wal_path = "wal.log");

    // 析构函数
    ~KVStore();

    // SET
    void set(const std::string& key, const std::string& value, bool log = true);
    
    // 设置带过期时间的键值对
    void set_with_ttl(const std::string& key, const std::string& value, std::chrono::seconds ttl, bool log = true);

    // GET
    std::string get(const std::string& key);

    // DEL
    bool del(const std::string& key, bool log = true);

    // 获取存储大小
    size_t size() const; // 常量成员函数

    // 检查键是否存在
    bool exists(const std::string& key) const;

    // 获取所有键
    std::vector<std::string> keys() const;

private:
    std::unordered_map<std::string, std::pair<std::string, std::chrono::steady_clock::time_point>> data_; // 数据存储 + 过期时间戳
    mutable std::mutex mutex_; // 互斥锁，用mutable修饰，即使是const依旧可以修改
    WAL* wal; //持久化日志系统类指针
    std::atomic<bool> ttl_cleanup_running_; // TTL清理线程状态
    std::thread ttl_cleanup_thread_; // TTL清理线程

    // 清理过期键
    void cleanup_expired_keys();

    // 禁止拷贝构造和赋值
    KVStore(const KVStore&) = delete;
    KVStore& operator=(const KVStore&) = delete;
};


#endif // KVSTORE_H