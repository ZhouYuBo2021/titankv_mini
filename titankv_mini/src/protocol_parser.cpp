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
        std::string key, value;
        iss >> key;

        // 获取剩余本分为value
        std::getline(iss, value);

        // 去除前导空格
        value.erase(0, value.find_first_not_of(" \t"));

        if (key.empty() || value.empty())
        {
            return "ERR SET requires key and value\n";
        }

        return parse_set(store, key, value);
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