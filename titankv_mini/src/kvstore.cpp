#include "../include/kvstore.h"
#include "../include/wal.h"
#include <vector>
#include <algorithm>
#include <iostream>
#include <thread>

// 构造函数
KVStore::KVStore(const std::string& wal_path) : wal(nullptr), ttl_cleanup_running_(false)
{
    try {
        wal = new WAL(wal_path);
        wal->replay(*this); // 启动时恢复数据

        // 启动TTL清理线程
        ttl_cleanup_running_ = true;
        ttl_cleanup_thread_ = std::thread(&KVStore::cleanup_expired_keys, this);
    } catch (const std::exception& e) {
        std::cerr << "KVStore initialization error: " << e.what() << std::endl;
        if (wal) {
            delete wal;
            wal = nullptr;
        }
        throw;
    }
}

KVStore::~KVStore()
{
    // 停止TTL清理线程
    ttl_cleanup_running_ = false;
    if (ttl_cleanup_thread_.joinable())
    {
        ttl_cleanup_thread_.join();
    }

    if (wal)
    {
        delete wal;
        wal = nullptr;
    }
}

// SET
void KVStore::set(const std::string& key, const std::string& value, bool log)
{
    if (key.empty())
    {
        throw std::invalid_argument("Key cannot be empty");
    }
    
    std::lock_guard<std::mutex> lock(mutex_);

    // 记录到WAL
    if (log && wal)
    {
        wal->log_set(key, value);
    }

    // 更新数据,并设置永不过期
    data_[key] = {value, std::chrono::steady_clock::time_point::max()};
}

// 设置带有过期时间的键值对
void KVStore::set_with_ttl(const std::string& key, const std::string& value, std::chrono::seconds ttl, bool log = true)
{
    if (key.empty())
    {
        throw std::invalid_argument("Key cannot be empty");
    }

    if (ttl.count() < 0)
    {
        throw std::invalid_argument("TTL must be positive");
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // 记录到WAL
    if (log && wal)
    {
        wal->log_set(key, value);
        // 记录TTL信息
        wal->log_ttl(key, ttl.count());
    }

    // 设置值和过期时间
    auto expiry_time = std::chrono::steady_clock::now() + ttl; // 在将来某个时间点过期，这个时间点就是expiry_time，既是一个时间戳
    data_[key] = {value, expiry_time};
}

// 清理过期键
void KVStore::cleanup_expired_keys()
{
    while (ttl_cleanup_running_)
    {
        // 每10秒检查一次过期键
        std::this_thread::sleep_for(std::chrono::seconds(10));

        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        std::vector<std::string> expired_key;

        // 找出所有过期的键
        for (const auto& pair : data_)
        {
            if (pair.second.second < now)
            {
                expired_key.push_back(pair.first);
            }
        }

        // 删除过期的键
        for (const auto& key : expired_key)
        {
            data_.erase(key);
            // 记录操作到WAL
            if (wal) 
            {
                wal->log_del(key);
            }
        }
    }
}

// GET
std::string KVStore::get(const std::string& key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it != data_.end())
    {
        // 检查键是否过期
        auto now = std::chrono::steady_clock::now();
        if (it->second.second < now)
        {
            // 已过期，删除并返回空
            data_.erase(it);
            // 记录删除操作到WAL
            if (wal) {
                wal->log_del(key);
            }
            return "";
        }

        return it->second.first;
    }

    return "";
}

// DEL
bool KVStore::del(const std::string& key, bool log)
{
    if (key.empty())
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = data_.find(key);
    if (it != data_.end())
    {
        if (log && wal)
        {
            wal->log_del(key);
        }

        data_.erase(it);
        return true;
    }

    return false;
}

// 获取存储大小
size_t KVStore::size() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.size();
}

// exist
bool KVStore::exists(const std::string& key) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.find(key) != data_.end();
}

// 获取所有键
std::vector<std::string> KVStore::keys() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> key_list;
    key_list.reserve(data_.size());

    for (const auto& pair : data_)
    {
        key_list.push_back(pair.first);
    }

    return key_list;
}