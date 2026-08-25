# ============================================================
# Makefile for LibertyParser - 优化版
# 
# 用法:
#   make                - 编译 Debug 版本（带调试信息）
#   make release        - 编译 Release 版本（优化，无调试信息）
#   make clean          - 清理所有构建产物
#   make rebuild        - 清理后重新编译
#   make -j 4           - 使用 4 个核心并行编译
#   make MODE=release   - 强制指定编译模式
# ============================================================

# ----- 编译器与标准设置 -----
CXX       = g++
CXXFLAGS  = -std=c++17 -Wall -Wextra -MMD -MP
TARGET    = LibertyParser

# ----- 源文件列表 -----
SRCS = LibertyParser.cpp \
       pathAnalyzer.cpp \
       cfgFileLoader.cpp \
       main.cpp

# ----- 自动生成目标文件列表（放在 build/ 目录下）-----
OBJDIR    = build
OBJS      = $(addprefix $(OBJDIR)/, $(SRCS:.cpp=.o))
DEPS      = $(OBJS:.o=.d)

# ----- 编译模式控制（默认 Debug）-----
ifeq ($(MODE), release)
    CXXFLAGS += -O2 -DNDEBUG
    BUILD_TYPE = release
else
    CXXFLAGS += -g -O0
    BUILD_TYPE = debug
endif

# ----- 默认目标：编译 Debug 版本 -----
all: $(OBJDIR) $(TARGET)

# ----- 创建构建目录 -----
$(OBJDIR):
	@mkdir -p $(OBJDIR)

# ----- 链接：生成最终可执行文件 -----
$(TARGET): $(OBJS)
	@echo "  [LD]    $@ ($(BUILD_TYPE))"
	$(CXX) $(CXXFLAGS) -o $@ $^

# ----- 编译规则：将任意 .cpp 编译为 .o（放在 build/ 下）-----
# $< 表示第一个依赖文件（.cpp）
# $@ 表示目标文件（.o）
$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	@echo "  [CXX]   $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ----- 自动依赖生成（通过 -MMD -MP 生成 .d 文件）-----
# .d 文件会被自动包含，实现头文件变更时的增量编译
-include $(DEPS)

# ----- 清理：删除构建目录和可执行文件 -----
clean:
	@echo "  [CLEAN] Removing build files..."
	@rm -rf $(OBJDIR) $(TARGET)
	@echo "  [CLEAN] Done."

# ----- 完全重建 -----
rebuild: clean all

# ----- Release 模式快捷入口 -----
release: MODE = release
release: clean all

# ----- 声明伪目标（防止与文件名冲突）-----
.PHONY: all clean rebuild release

# ============================================================
# 高级功能：打印当前编译配置（调试用）
# ============================================================
info:
	@echo "=== Build Configuration ==="
	@echo "CXX        : $(CXX)"
	@echo "MODE       : $(BUILD_TYPE)"
	@echo "CXXFLAGS   : $(CXXFLAGS)"
	@echo "SRCS       : $(SRCS)"
	@echo "OBJS       : $(OBJS)"
	@echo "TARGET     : $(TARGET)"
	@echo "==========================="