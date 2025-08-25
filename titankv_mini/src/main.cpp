#include <iostream>
#include <string>
#include <csignal>
#include <thread>
#include <chrono>

#include "../include/kvstore.h"
#include "../include/network_server.h"
#include "../include/protocol_parser.h"

// 全局变量用于信号处理
static std::atomic<bool> g_running(true);

// 信号处理函数
void signal_handler(int signal)
{
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    g_running.store(false);
}

// 显示帮助信息
void show_help()
{
    std::cout << "TitanKV Mini - Simple Key-Value Store\n";
    std::cout << "Usage: ./titankv_mini [port] [wal_file]\n";
    std::cout << "\nCommands:\n";
    std::cout << "  SET <key> <value> - Store a key-value pair\n";
    std::cout << "  GET <key>         - Retrieve a key-value pair\n";
    std::cout << "  DEL <key>         - Delete a key-value pair\n";
    std::cout << "\nInteractive command:\n";
    std::cout << "  help              - Show this help\n";
    std::cout << "  stats             - Show store statistics\n";
    std::cout << "  exit              - Stop the server\n";
}

// 显示存储统计信息
void show_stats(KVStore& store)
{
    std::cout << "Store Statistics:\n";
    std::cout << "  Total keys: " << store.size() << "\n"; 
}

int main(int argc, char* argv[])
{
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 解析命令行参数
    int port = 6380;
    std::string wal_path = "wal.log";

    if (argc > 1)
    {
        try {
            port = std::stoi(argv[1]);
            if (port < 1 || port > 65535)
            {
                std::cerr << "Error: Port must be between 1 and 65535" << std::endl;
                return 1;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: Invalid port number" << std::endl;
            return 1;
        }
    }

    if (argc > 2) {
        wal_path = argv[2];
    }

    try {
        // 创建KV存储
        KVStore store(wal_path);

        // 创建网络服务器
        NetworkServer server(store, port);

        std::cout << "Starting TitanKV mini server...\n";
        std::cout << "Port: " << port << "\n";
        std::cout << "WAL file: " << wal_path << "\n";
        show_help();

        // 启动服务器
        server.start();

        // 等待推出命令
        std::string command;
        while (g_running.load())
        {
            std::cout << "titan>\n";
            std::getline(std::cin, command);
            //std::cout.flush();
            //if (!)
           // {
                // 如果读取失败（如EOF或错误），退出循环
              //  break;
           // }

            // 去除前后空格
            command.erase(0, command.find_first_not_of(" \t"));
            command.erase(command.find_last_not_of(" \t") + 1);

            if (command.empty())
            {
                continue;
            }

            if (command == "exit" || command == "quit")
            {
                break;
            } else if (command == "help")
            {
                show_help();
            }else if (command == "stats")
            {
                show_stats(store);
            }else 
            {
                // 处理其他命令
                std::string response = ProtocolParser::parse(store, command);
                std::cout << response;
            }
        }

        std::cout << "Shutting down server..." << std::endl;
        server.stop();

    } catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Server stopped successfully." << std::endl;
    return 0;
}