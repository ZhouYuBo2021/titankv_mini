#!/bin/bash

# TitanKV Mini 构建脚本

echo "开始构建 TitanKV Mini..."

# 检查g++是否安装
if ! command -v g++ &> /dev/null; then
    echo "错误: g++ 未安装，请先安装g++"
    exit 1
fi

# 检查make是否安装
if ! command -v make &> /dev/null; then
    echo "错误: make 未安装，请先安装make"
    exit 1
fi

# 创建构建目录
mkdir -p build

# 编译项目
echo "使用Makefile编译..."
make clean
make

# 检查编译结果
if [ -f "titankv_mini" ]; then
    echo "构建成功! 可执行文件: ./titankv_mini"
    chmod +x titankv_mini
else
    echo "构建失败!"
    exit 1
fi

# 显示文件大小
ls -lh titankv_mini

echo "构建完成!"