#include "../include/network_server.h"
#include "../include/protocol_parser.h"
#include "../include/kvstore.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <cerrno>

// 构造函数
NetworkServer::NetworkServer(KVStore& store, int port)
    : store_(store), port_(port), running_(false), server_fd_(-1)
{
}

// 析构函数
NetworkServer::~NetworkServer()
{
    stop();
}

// 启动服务器
void NetworkServer::start()
{
    if (running_.load())
    {
        std::cerr << "Server is already running" << std::endl;
		return;
    }

    running_.store(true);
    server_thread_ = std::thread(&NetworkServer::run, this);
}

// 停止服务器
void NetworkServer::stop()
{
    if (!running_.load())
    {
        return;
    }

    running_.store(false);

    // 关闭服务器socket以中断accept调用
    if (server_fd_ >= 0)
    {
        shutdown(server_fd_, SHUT_RDWR);
        close(server_fd_);
        server_fd_ = -1;
    }

    if (server_thread_.joinable())
    {
        server_thread_.join();
    }
}

// 运行服务器主循环
void NetworkServer::run()
{
    // 创建socket
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0)
    {
        std::cerr << "Socket creation failed: " << strerror(errno) << std::endl;
        return;
    }

    // 设置socket选项，允许地址重用
    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        std::cerr << "Setsockopt failed: " << strerror(errno) << std::endl;
        close(server_fd_);
        return;
    }

    // 设置accept超时时间
    struct timeval timeout;
    timeout.tv_sec = 1; // 一秒超时
    timeout.tv_usec = 0;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        std::cerr << "Failed to set socket timeout: " << strerror(errno) << std::endl;
    }

    sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    // 绑定socket
    if (bind(server_fd_, (sockaddr*)&address, sizeof(address)) < 0)
    {
        std::cerr << "Bind failed: " << strerror(errno) << std::endl;
        close(server_fd_);
        server_fd_ = -1;
        return;
    }

    // 监听连接
    if (listen(server_fd_, 10) < 0)
    {
        std::cerr << "Listen failed: " << strerror(errno) << std::endl;
        close(server_fd_);
        server_fd_ = -1;
        return;
    }

    std::cout << "TitanKV mini running on port " << port_ << std::endl;

    while (running_.load())
    {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        // 接收客户端连接
        int client_fd = accept(server_fd_, (sockaddr*)&client_addr, &client_len);

        if (client_fd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) 
            {
                // 超时，继续循环
                continue;
            } 
            else if (errno != EINTR) 
            {
                std::cerr << "Accept failed: " << strerror(errno) << std::endl;
                continue;
            }
            continue;
        }

        // 创建新线程处理客户端
        std::thread([this, client_fd] {
            handle_client(client_fd);
            close(client_fd);
        }).detach();
    }

    close(server_fd_);
    server_fd_ = -1;
}

// 处理客户端连接
void NetworkServer::handle_client(int client_fd)
{
    char buffer[1024];

    while (running_.load())
    {
        memset(buffer, 0, sizeof(buffer));

        // 从客户端套接字读取数据
        int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);

        if (bytes_read == 0)
        {
            // 客户端断开连接
            break;
        }

        if (bytes_read < 0)
        {
            if (errno == EINTR) {
                continue;
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            } else {
                std::cerr << "Read error: " << strerror(errno) << std::endl;
                break;
            }
        }

        // 解析并处理请求
        std::string request(buffer, bytes_read);

        // 去除末尾的换行和回车符
        while (!request.empty() && (request.back() == '\n' || request.back() == '\r'))
        {
            request.pop_back();
        }

        if (!request.empty())
        {
            std::string response = ProtocolParser::parse(store_, request);

            // 将处理结果发送回客户端
            ssize_t bytes_sent = send(client_fd, response.c_str(), response.size(), 0);
            if (bytes_sent < 0) {
                std::cerr << "Send error: " << strerror(errno) << std::endl;
                break;
            }
        }
    }
}