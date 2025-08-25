#include "../include/wal.h"
#include "../include/kvstore.h"
#include <stdexcept>
#include <iostream>

// 构造函数
WAL::WAL(const std::string& path) : log_path(path), last_flush_size(0)
{
    // 以追加模式和二进制模式打开日志文件
    log_file.open(log_path, std::ios::app | std::ios::binary);
    // 检查是否打开成功
    if (!log_file.is_open())
    {
        throw std::runtime_error("Failed to open WAL file: " + log_path);
    }
    // 将文件指针移动到文件末尾
    log_file.seekp(0, std::ios::end);
    // 记录当前文件大小
    last_flush_size = log_file.tellp();
}

// 析构函数
WAL::~WAL()
{
    if (log_file.is_open())
    {
        // 强制刷新缓冲区到磁盘
        log_file.flush();
        log_file.close();
    }
}

// log_set
void WAL::log_set(const std::string& key, const std::string& value)
{
    // 加锁保护线程安全
    std::lock_guard<std::mutex> lock(log_mutex);
    // 构建日志条目
    std::string entry = "SET " + key + " " + value + "\n";
    // 将条目写入文件
    log_file << entry;
    // 立即刷新缓冲区，确保数据持久化到硬盘
    log_file.flush();

    // 检查是否写入成功
    if (!log_file.good())
    {
        throw std::runtime_error("Failed to write to WAL file");
    }
}

// log_del
void WAL::log_del(const std::string& key)
{
    std::lock_guard<std::mutex> lock(log_mutex);
    std::string entry = "DEL " + key + "\n";
    log_file << entry;
    log_file.flush();

    if (!log_file.good())
    {
        throw std::runtime_error("Failed to write to WAL file");
    }
}

// 重放日志以恢复数据
void WAL::replay(KVStore& store)
{
    std::lock_guard<std::mutex> lock(log_mutex);

    // 如果文件没有打开，直接返回
    /*if (!log_file.is_open()) {
        log_file.open(log_path, std::ios::app | std::ios::binary);
        if (!log_file.is_open()) {
            throw std::runtime_error("Failed to reopen WAL file");
        }
        return;
    }*/

    // 如果写文件流是打开的，先关闭它
    if (log_file.is_open()) {
        log_file.close();
    }
    
    // 关闭当前输出文件流，现在写入不需要了
    //log_file.close();
    
    // 以输入模式重新打开日志文件，以供读操作
    std::ifstream infile(log_path, std::ios::binary);
    if (!infile.is_open())
    {
        // 如果打开失败，那么重新打开输出文件流
        log_file.open(log_path, std::ios::app | std::ios::binary);
        if (!log_file.is_open()) {
            throw std::runtime_error("Failed to reopen WAL file for writing");
        }
        return;
    }

    std::string line;     // 存储每行数据
    int count = 0;        // 记录恢复的操作数量
    int line_number = 0;  // 记录行号用于错误报告

    // 一行一行来
    while (std::getline(infile, line))
    {
        line_number++;

        // 空行跳过
        if (line.empty()) continue;

        size_t pos = line.find(' '); // 找到命令和参数之间的空格
        if (pos == std::string::npos)
        {
            std::cerr << "Warning: Invalid Wal entry at line" << line_number << ", skipping" << std::endl;
            continue;
        }

        std::string cmd = line.substr(0, pos); // 提取命令
        std::string args = line.substr(pos + 1); // 提取参数

        // 处理cmd
        if (cmd == "SET")
        {
            // 找到key 和value 之间的空格
            size_t key_end = args.find(' ');
            if (key_end != std::string::npos)
            {
                std::string key = args.substr(0, key_end);
                std::string value = args.substr(key_end + 1);
                // 调用KV存储引擎进行SET操作,且重放时不需要记录日志
                store.set(key, value, false);
                count++;
            } else {
                std::cerr << "Warning: Invalid SET format at line " << line_number << ", skiping" << std::endl; 
            }
        } else if (cmd == "DEL")
        {
            store.del(args, false);
            count++;
        } else 
        {
            std::cerr << "Warning: Unkown command" << cmd << "at line" << line_number << ", skipping" << std::endl;
        }
    }

    infile.close();

    std::cout << "WAL recovery completed. Restored " << count << " operations." << std::endl;

    // 重新打开文件为追加模式
    log_file.open(log_path, std::ios::app | std::ios::binary);
    if (!log_file.is_open())
    {
        throw std::runtime_error("Failed to reopen WAL file for appending");
    }
    last_flush_size = log_file.tellp();
}