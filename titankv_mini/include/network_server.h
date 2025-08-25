#ifndef NETWORK_SERVER_H
#define NETWORK_SERVER_H

#include <thread>
#include <atomic>
#include <string>

// 前向声明
class KVStore;

class NetworkServer {
public:
    // 构造函数
    NetworkServer(KVStore& store, int port = 6379);
    
    // 析构函数
    ~NetworkServer();

    // 启动服务器
    void start();

    // 停止服务器
    void stop();

    // 检查服务器是否在运行
    bool is_running() const { return running_.load(); }

    // 获取服务器端口
    int get_port() const { return port_; } 



private:
    // 运行服务器主循环
    void run();

    // 处理客户端连接
    void handle_client(int client_fd);

    KVStore& store_;                // KV存储引用
    int port_;                      // 服务器端口
    std::atomic<bool> running_;     // 运行标志
    std::thread server_thread_;     // 服务器线程
    int server_fd_;                 // 服务器socket描述符

    // 禁止拷贝构造和赋值
    NetworkServer(const NetworkServer&) = delete;
    NetworkServer& operator=(const NetworkServer&) = delete;
};

#endif // NETWORK_SERVER_H