# Makefile for TitanKV Mini

# 编译器
CXX = g++

# 编译选项
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -g

# 链接选项
LDFLAGS = -pthread

# 目录
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build

# 源文件
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# 目标文件
TARGET = titankv_mini

# 默认目标
all: $(TARGET)

# 编译可执行文件
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# 编译对象文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) -c $< -o $@

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 清理生成的文件
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# 运行程序
run: $(TARGET)
	./$(TARGET)

# 调试版本
debug: CXXFLAGS += -DDEBUG -O0
debug: $(TARGET)

# 发布版本
release: CXXFLAGS += -DNDEBUG -O3
release: clean $(TARGET)

# 安装
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

# 卸载
uninstall:
	rm -f /usr/local/bin/$(TARGET)

# 显示帮助
help:
	@echo "Available targets:"
	@echo "  all     - Build the executable (default)"
	@echo "  clean   - Remove build artifacts"
	@echo "  run     - Build and run the program"
	@echo "  debug   - Build with debug symbols"
	@echo "  release - Build optimized release"
	@echo "  install - Install to /usr/local/bin"
	@echo "  help    - Show this help"

# 声明伪目标
.PHONY: all clean run debug release install uninstall help