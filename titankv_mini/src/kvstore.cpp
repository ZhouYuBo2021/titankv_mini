#include "../include/kvstore.h"
#include "../include/wal.h"
#include <vector>
#include <algorithm>
#include <iostream>

// 构造函数
KVStore::KVStore(const std::string& wal_path) : wal(nullptr)
{
    try {
        wal = new WAL(wal_path);
        wal->replay(*this); // 启动时恢复数据
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

    // 更新数据
    data_[key] = value;
}

// GET
std::string KVStore::get(const std::string& key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    return it != data_.end() ? it->second : "NOT_FOUND";
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