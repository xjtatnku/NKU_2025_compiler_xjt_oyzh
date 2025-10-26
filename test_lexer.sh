#!/bin/bash

echo "========================================"
echo "  词法分析器 (Lab1) 测试"
echo "========================================"
echo ""

cd /mnt/c/Users/xjt26/Desktop/NKU-Compiler2025-main

# 测试1: simple.sy
echo "[测试1] 测试用例: simple.sy"
./bin/compiler -lexer testcase/lexer/simple.sy -o output1.txt 2>&1
if [ -z "$(diff testcase/lexer/simple.lexer output1.txt)" ]; then
    echo "✅ simple.sy - 通过"
else
    echo "❌ simple.sy - 失败"
    echo "查看差异: diff testcase/lexer/simple.lexer output1.txt"
fi
echo ""

# 测试2: witharray.sy
echo "[测试2] 测试用例: witharray.sy"
./bin/compiler -lexer testcase/lexer/witharray.sy -o output2.txt 2>&1
if [ -z "$(diff testcase/lexer/witharray.lexer output2.txt)" ]; then
    echo "✅ witharray.sy - 通过"
else
    echo "❌ witharray.sy - 失败"
    echo "查看差异: diff testcase/lexer/witharray.lexer output2.txt"
fi
echo ""

# 测试3: withfloat.sy
echo "[测试3] 测试用例: withfloat.sy"
./bin/compiler -lexer testcase/lexer/withfloat.sy -o output3.txt 2>&1
if [ -z "$(diff testcase/lexer/withfloat.lexer output3.txt)" ]; then
    echo "✅ withfloat.sy - 通过"
else
    echo "❌ withfloat.sy - 失败"
    echo "查看差异: diff testcase/lexer/withfloat.lexer output3.txt"
fi
echo ""

echo "========================================"
echo "  所有测试完成"
echo "========================================"

