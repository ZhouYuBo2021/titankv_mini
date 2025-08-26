#include "../include/protocol_parser.h"
#include "../include/kvstore.h"
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>

// 解析协议请求
std::string ProtocolParser::parse(KVStore& store, const std::string& request)
{
    if (request.empty())
    {
        return "ERR empty request\n";
    }

    // 使用字符串流来解析命令
    std::istringstream iss(request);
    std::string command;
    iss >> command;

    if (command.empty())
    {
        return "ERR invalid command\n";
    }

    // 转换为大写以便不区分大小写
    std::transform(command.begin(), command.end(), command.begin(), ::toupper);
    // 去除命令中的空白字符
    command.erase(std::remove(command.begin(), command.end(), ' '), command.end());

    if (command == "SET")
    {
        std::string key, value_part, ttl_str;
        iss >> key;

        // 检查是否有TTL参数
        bool has_ttl = false;
        int64_t ttl_seconds = 0;

        // 获取剩余部分
        std::string rest;
        std::getline(iss, rest);

        // 查找TTL关键字
        size_t ttl_pos = rest.find("TTL");
        if (ttl_pos != std::string::npos)
        {
            // 提取value部分
            value_part = rest.substr(0, ttl_pos);
            // 去除前导和尾随空格
            value_part.erase(0, value_part.find_first_not_of(" \t"));
            value_part.erase(value_part.find_last_not_of(" \t") + 1);

            // 提取TTL值
            std::istringstream ttl_iss(rest.substr(ttl_pos + 3)); // +3 跳过 "TTL"
            ttl_iss >> ttl_str;

            try {
                ttl_seconds = std::stoll(ttl_str);
                has_ttl = true;
            } catch (const std::invalid_argument& e) {
                return "ERR invalid TTL value\n";
            } catch (const std::out_of_range& e) {
                return "ERR TTL value out of range\n";
            }

        }else
        {
            // 没有TTL参数
            value_part = rest;
            // 去除前导空格
            value_part.erase(0, value_part.find_first_not_of(" \t"));
        }

        if (key.empty() || value_part.empty()) {
            return "ERR SET requires key and value\n";
        }
        
        if (has_ttl) {
            if (ttl_seconds <= 0) {
                return "ERR TTL must be positive\n";
            }
            return parse_set_with_ttl(store, key, value_part, ttl_seconds);
        } else {
            return parse_set(store, key, value_part);
        }
    }
    else if (command == "GET")
    {
        std::string key;
        iss >> key;
        if (key.empty())
        {
            return "ERR GET requires key\n";
        }

        return parse_get(store, key);
    }
    else if (command == "DEL")
    {
        std::string key;
        iss >> key;
        if (key.empty())
        {
            return "ERR DEL requires key\n";
        }

        return parse_del(store, key);
    }

    return "ERR unkown command '" + command + "'\n";
}

// 检查命令是否有效
bool ProtocolParser::is_valid_command(const std::string& command)
{
    std::string cmd = command;
    // 提取命令部分（忽略参数）
    size_t space_pos = cmd.find(' ');
    if (space_pos != std::string::npos) {
        cmd = cmd.substr(0, space_pos);
    }
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    return cmd == "SET" || cmd == "GET" || cmd == "DEL";
}

// 获取命令类型
std::string ProtocolParser::get_command_type(const std::string& request)
{
    std::istringstream iss(request);
    std::string command;
    iss >> command;
    
    if (!command.empty())
    {
        std::transform(command.begin(), command.end(), command.begin(), ::toupper);
        return command;
    }

    return "";
}

// 解析SET命令
std::string ProtocolParser::parse_set(KVStore& store, const std::string& key, const std::string& value)
{
    try {
        store.set(key, value);
        return "OK\n";
    } catch (const std::exception& e)
    {
        return "ERR " + std::string(e.what()) + "\n";
    }
}

// 接下带有TTL的SET命令
std::string ProtocolParser::parse_set_with_ttl(KVStore& store, const std::string& key, const std::string& value, int64_t ttl_seconds)
{
    try {
        store.set_with_ttl(key, value, std::chrono::seconds(ttl_seconds));
        return "OK\n";
    } catch (const std::exception& e)
    {
        return "ERR " + std::string(e.what()) + "\n"; 
    }

}

// 解析GET命令
std::string ProtocolParser::parse_get(KVStore& store, const std::string& key)
{
    try {
        std::string value = store.get(key);
        if (value == "NOTFOUND") {
            return "NOT_FOUND\n";
        }
        return value + "\n";
    } catch (const std::exception& e) {
        return "ERR " + std::string(e.what()) + "\n";
    }
}

// 解析DEL命令
std::string ProtocolParser::parse_del(KVStore& store, const std::string& key)
{
    try {
        bool deleted = store.del(key);
        return deleted ? "OK\n" : "NOT_FOUND\n";
    } catch (const std::exception& e) {
        return "ERR " + std::string(e.what()) + "\n";
    }
}

// 解析错误响应
std::string ProtocolParser::parse_error(const std::string& message)
{
    return "ERR " + message + "\n";
}