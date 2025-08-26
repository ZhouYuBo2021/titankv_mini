#ifndef PROTOCOL_PARSER_H
#define PROTOCOL_PARSER_H

#include <string>

class KVStore;

class ProtocolParser {
public:
    // 解析协议请求
    static std::string parse(KVStore& store, const std::string& request);

    // 检查命令是否有效
    static bool is_valid_command(const std::string& command);

    // 获取命令类型
    static std::string get_command_type(const std::string& request);

private:
    // 解析SET命令
    static std::string parse_set(KVStore& store, const std::string& key, const std::string& value);

    // 解析带有TTL的SET命令
    static std::string parse_set_with_ttl(KVStore& store, const std::string& key, const std::string& value, int64_t ttl_seconds);

    // 解析GET命令
    static std::string parse_get(KVStore& store, const std::string& key);

    // 解析DEL命令
    static std::string parse_del(KVStore& store, const std::string& key);

    // 解析错误响应
    static std::string parse_error(const std::string& message);
};


#endif // PROTOCOL_PARSER_H