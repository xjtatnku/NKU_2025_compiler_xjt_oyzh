#!/bin/bash

# 语义检查测试脚本
# 测试编译器能否正确识别语义错误

COMPILER="./bin/compiler"
TESTCASE_DIR="testcase/semant"
PASSED=0
FAILED=0
TOTAL=0

echo "================================"
echo "  语义检查测试 (Semantic Check)"
echo "================================"
echo ""

# 颜色定义
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# 遍历所有.sy文件
for test_file in "$TESTCASE_DIR"/*.sy; do
    if [ ! -f "$test_file" ]; then
        continue
    fi
    
    TOTAL=$((TOTAL + 1))
    test_name=$(basename "$test_file")
    
    # 运行编译器，预期应该失败（返回非0退出码）
    "$COMPILER" "$test_file" -llvm -o /tmp/test_output.ll > /dev/null 2>&1
    exit_code=$?
    
    # 检查是否正确识别了错误（exit_code应该不为0）
    if [ $exit_code -ne 0 ]; then
        echo -e "${GREEN}[PASS]${NC} $test_name - 编译器正确识别了语义错误"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}[FAIL]${NC} $test_name - 编译器未能识别语义错误"
        FAILED=$((FAILED + 1))
    fi
done

echo ""
echo "================================"
echo "  测试结果统计"
echo "================================"
echo "总计: $TOTAL"
echo -e "${GREEN}通过: $PASSED${NC}"
echo -e "${RED}失败: $FAILED${NC}"

if [ $TOTAL -gt 0 ]; then
    pass_rate=$(awk "BEGIN {printf \"%.2f\", ($PASSED/$TOTAL)*100}")
    echo "通过率: ${pass_rate}%"
fi
echo "================================"

# 如果有失败的测试，返回1
if [ $FAILED -gt 0 ]; then
    exit 1
else
    exit 0
fi
