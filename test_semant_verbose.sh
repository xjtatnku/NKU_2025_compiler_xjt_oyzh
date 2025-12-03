#!/bin/bash

# 语义检查详细测试脚本
# 显示每个测试用例的详细错误信息

COMPILER="./bin/compiler"
TESTCASE_DIR="testcase/semant"
PASSED=0
FAILED=0
TOTAL=0

echo "========================================"
echo "  语义检查详细测试 (Semantic Check)"
echo "========================================"
echo ""

# 颜色定义
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 遍历所有.sy文件
for test_file in "$TESTCASE_DIR"/*.sy; do
    if [ ! -f "$test_file" ]; then
        continue
    fi
    
    TOTAL=$((TOTAL + 1))
    test_name=$(basename "$test_file")
    
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${CYAN}测试: $test_name${NC}"
    echo ""
    
    # 显示测试文件内容
    echo -e "${YELLOW}源代码:${NC}"
    cat "$test_file" | head -15
    echo ""
    
    # 运行编译器，捕获输出
    echo -e "${YELLOW}编译器输出:${NC}"
    output=$("$COMPILER" "$test_file" -llvm -o /tmp/test_output.ll 2>&1)
    exit_code=$?
    
    # 只显示错误相关的行
    echo "$output" | grep -E "(Error:|Semantic|failed|Assertion)" || echo "$output" | tail -5
    echo ""
    
    # 检查是否正确识别了错误
    if [ $exit_code -ne 0 ]; then
        echo -e "${GREEN}✓ PASS${NC} - 编译器正确识别了语义错误 (exit code: $exit_code)"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}✗ FAIL${NC} - 编译器未能识别语义错误"
        FAILED=$((FAILED + 1))
    fi
    echo ""
done

echo -e "${BLUE}========================================${NC}"
echo -e "${CYAN}测试结果统计${NC}"
echo -e "${BLUE}========================================${NC}"
echo "总计: $TOTAL"
echo -e "${GREEN}通过: $PASSED${NC}"
echo -e "${RED}失败: $FAILED${NC}"

if [ $TOTAL -gt 0 ]; then
    pass_rate=$(awk "BEGIN {printf \"%.2f\", ($PASSED/$TOTAL)*100}")
    echo "通过率: ${pass_rate}%"
fi
echo -e "${BLUE}========================================${NC}"

# 如果有失败的测试，返回1
if [ $FAILED -gt 0 ]; then
    exit 1
else
    exit 0
fi
