INC_DIR := . interfaces utils
SRC_DIR := frontend middleend backend interfaces utils
OBJ_DIR := obj
BIN_DIR := bin

# ============================================================================
# 并行编译优化配置
# ============================================================================
# 自动检测CPU核心数（用于info命令显示和默认并行任务数）
ifeq ($(OS),Windows_NT)
    # Windows系统：检测逻辑处理器数量
    NPROC := $(shell powershell -Command "(Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors" 2>nul || echo 4)
    DEFAULT_JOBS := $(NPROC)
else
    # Linux/Mac系统：使用nproc检测（WSL/Linux环境）
    NPROC := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    DEFAULT_JOBS := $(NPROC)
endif

# ============================================================================
# 编译器配置
# ============================================================================
# 优先使用clang++，如果不存在则使用g++
CXX := $(shell which clang++ 2>/dev/null || which g++ 2>/dev/null || echo g++)
CXX_NAME := $(shell $(CXX) --version 2>/dev/null | head -1 | cut -d' ' -f1 || echo "Unknown")
INCLUDES = $(addprefix -I, $(INC_DIR))
CXX_STANDARD = -std=c++17
DBGFLAGS = -g
# 一些编译器（如 clang）不支持 -Wno-unused-but-set-variable，会导致 unknown-warning-option 报错
# 将其从默认警告选项中移除，以保证在不同编译器下都能正常编译
WERROR_FLAGS := -Wall -Wextra -Wpedantic -Werror
WARN_IGNORE := 
CUSTOM_FLAGS := -DLOCAL_TEST
CXXFLAGS = -O2 -MMD -MP $(CXX_STANDARD) $(INCLUDES) $(WERROR_FLAGS) $(DBGFLAGS) $(WARN_IGNORE) $(CUSTOM_FLAGS)

SOURCES = $(shell find $(SRC_DIR) -name "*.cpp" -type f)
MAIN_SRC = main.cpp

OBJECTS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SOURCES))
MAIN_OBJ = $(OBJ_DIR)/main.o
GEN_SRCS = $(BISON_C) $(LEXER_C)
GEN_OBJECTS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(GEN_SRCS))
ALL_OBJECTS = $(OBJECTS) $(GEN_OBJECTS) $(MAIN_OBJ)

TARGET = $(BIN_DIR)/compiler

PARSER_DIR = frontend/parser
LEXER_SRC  = $(PARSER_DIR)/lexer.l
BISON_SRC = $(PARSER_DIR)/yacc.y

LEXER_C    = $(LEXER_SRC:.l=.cpp)
BISON_H   = $(BISON_SRC:.y=.h)
BISON_C   = $(BISON_SRC:.y=.cpp)
LOC_H     = $(PARSER_DIR)/location.hh

LEXER_FILES = $(LEXER_C) $(BISON_H) $(BISON_C) $(LOC_H)

SOURCES := $(filter-out $(GEN_SRCS), $(SOURCES))

# 默认编译目标（串行）
all: $(LEXER_FILES) $(TARGET)
	@echo "Build completed successfully!"
	@echo "Tip: Use 'make build-parallel' for faster compilation (uses $(DEFAULT_JOBS) parallel jobs)"

# 并行编译目标（推荐使用）
build-parallel: $(LEXER_FILES)
	@echo "Building with $(DEFAULT_JOBS) parallel jobs..."
	@$(MAKE) -j$(DEFAULT_JOBS) $(TARGET)
	@echo "Build completed successfully using $(DEFAULT_JOBS) parallel jobs!"

$(OBJ_DIR) $(BIN_DIR):
	@mkdir -p $@

$(OBJ_DIR)/%/:
	@mkdir -p $@

$(TARGET): $(ALL_OBJECTS) | $(BIN_DIR)
	@echo "[LD] Linking $(words $(ALL_OBJECTS)) object files -> $@"
	@$(CXX) $(ALL_OBJECTS) -o $@
	@echo "[OK] Build successful: $@"

$(OBJ_DIR)/main.o: main.cpp | $(OBJ_DIR)
	@echo "[CC] main.cpp"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@echo "[CC] $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(GEN_OBJECTS): $(LEXER_FILES)

-include $(ALL_OBJECTS:.o=.d)

# 检查clang-format是否可用
CLANG_FORMAT := $(shell which clang-format 2>/dev/null)
FORMAT_CMD := $(if $(CLANG_FORMAT),$(CLANG_FORMAT) -i,true)

$(BISON_H) $(BISON_C) $(LOC_H): $(BISON_SRC)
	@echo "Generating parser files from $(BISON_SRC)..."
	@bison -d --language=c++ --defines=$(BISON_H) -o $(BISON_C) $(BISON_SRC)
	@if [ -f $(BISON_H) ] && [ -n "$(CLANG_FORMAT)" ]; then $(CLANG_FORMAT) -i $(BISON_H); fi
	@if [ -f $(BISON_C) ] && [ -n "$(CLANG_FORMAT)" ]; then $(CLANG_FORMAT) -i $(BISON_C); fi
	@if [ -f $(LOC_H) ] && [ -n "$(CLANG_FORMAT)" ]; then $(CLANG_FORMAT) -i $(LOC_H); fi

$(LEXER_C): $(LEXER_SRC) $(BISON_H)
	@echo "Generating lexer file from $(LEXER_SRC)..."
	@flex --c++ --outfile=$(LEXER_C) $(LEXER_SRC)
	@if [ -f $(LEXER_C) ] && [ -n "$(CLANG_FORMAT)" ]; then $(CLANG_FORMAT) -i $(LEXER_C); fi

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

clean-lexer:
	rm -f $(LEXER_FILES)

lexer: $(LEXER_FILES)

format:
	@if [ -z "$(CLANG_FORMAT)" ]; then \
		echo "Error: clang-format not found. Install it with: sudo apt-get install clang-format"; \
		exit 1; \
	fi
	@echo "Formatting source files with clang-format..."
	@find . -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.hh" \) -exec $(CLANG_FORMAT) -i {} +
	@echo "Formatting completed."

# 显示系统信息和编译配置
info:
	@echo "=========================================="
	@echo "Build System Information"
	@echo "=========================================="
	@echo "Compiler: $(CXX) ($(CXX_NAME))"
	@echo "CPU cores: $(NPROC)"
	@echo "Default parallel jobs: $(DEFAULT_JOBS)"
	@echo "Memory: $(shell free -h 2>/dev/null | grep Mem | awk '{print $$2}' || echo 'N/A')"
	@echo ""
	@echo "Tools:"
	@echo "  bison: $(shell bison --version 2>/dev/null | head -1 || echo 'not found')"
	@echo "  flex:  $(shell flex --version 2>/dev/null | head -1 || echo 'not found')"
	@echo "  clang-format: $(if $(CLANG_FORMAT),$(shell $(CLANG_FORMAT) --version 2>/dev/null | head -1 || echo 'found'),not installed)"
	@echo ""
	@echo "Quick start (recommended):"
	@echo "  make build-parallel    # Uses $(DEFAULT_JOBS) parallel jobs automatically"
	@echo ""
	@echo "Manual parallel compilation:"
	@echo "  make -j$(DEFAULT_JOBS)  # Uses $(DEFAULT_JOBS) parallel jobs"
	@echo "  make -j8               # Uses 8 parallel jobs"
	@echo "  make -j16              # Uses 16 parallel jobs"
	@echo ""
	@echo "Other commands:"
	@echo "  make clean             # Clean build artifacts"
	@echo "  make format            # Format code (requires clang-format)"
	@echo "=========================================="

.PHONY: all clean clean-lexer lexer format info build-parallel
