#!/bin/bash

# TitanKV Mini 测试脚本

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 测试函数
test_command() {
    local cmd="$1"
    local expected="$2"
    
    echo -e "${YELLOW}测试: $cmd${NC}"
    response=$(echo "$cmd" | nc -w 2 localhost 6380)
    
    if [[ "$response" == *"$expected"* ]]; then
        echo -e "${GREEN}✓ 通过${NC}"
        return 0
    else
        echo -e "${RED}✗ 失败 - 期望: $expected, 实际: $response${NC}"
        return 1
    fi
}

# 清理旧文件
rm -f wal.log

# 启动服务器
echo "启动服务器..."
./titankv_mini 6380 wal.log > server.log 2>&1 &
SERVER_PID=$!

# 等待服务器启动
sleep 2

# 测试基本功能
echo "运行测试..."
test_command "SET test1 value1" "OK"
test_command "GET test1" "value1"
test_command "SET test2 value2" "OK"
test_command "GET test2" "value2"
test_command "DEL test1" "OK"
test_command "GET test1" "NOT_FOUND"
test_command "GET nonexistent" "NOT_FOUND"
test_command "INVALID_COMMAND" "ERR"

# 停止服务器
kill $SERVER_PID
wait $SERVER_PID

# 重启服务器测试数据持久化
echo "测试数据持久化..."
./titankv_mini 6380 wal.log > server2.log 2>&1 &
SERVER_PID=$!
sleep 2

test_command "GET test2" "value2"

# 停止服务器
kill $SERVER_PID
wait $SERVER_PID

echo -e "${GREEN}所有测试完成!${NC}"
echo "服务器日志:"
cat server.log 2>/dev/null || echo "无日志"

# 清理
rm -f server.log server2.log