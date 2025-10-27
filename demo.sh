#!/bin/bash

echo "========================================"
echo "  Lexical Analyzer Test"
echo "========================================"
echo ""

# Test 1: simple.sy
echo "[Test 1] simple.sy"
./bin/compiler -lexer testcase/lexer/simple.sy -o output1.txt 2>&1 | grep -v "Input file\|Step\|Output\|Optimize level"
if [ -z "$(diff testcase/lexer/simple.lexer output1.txt 2>&1)" ]; then
    echo "PASS: simple.sy"
else
    echo "FAIL: simple.sy"
    echo "Differences:"
    diff testcase/lexer/simple.lexer output1.txt | head -10
fi
echo ""

# Test 2: witharray.sy
echo "[Test 2] witharray.sy"
./bin/compiler -lexer testcase/lexer/witharray.sy -o output2.txt 2>&1 | grep -v "Input file\|Step\|Output\|Optimize level"
if [ -z "$(diff testcase/lexer/witharray.lexer output2.txt 2>&1)" ]; then
    echo "PASS: witharray.sy"
else
    echo "FAIL: witharray.sy"
    echo "Differences:"
    diff testcase/lexer/witharray.lexer output2.txt | head -10
fi
echo ""

# Test 3: withfloat.sy
echo "[Test 3] withfloat.sy"
./bin/compiler -lexer testcase/lexer/withfloat.sy -o output3.txt 2>&1 | grep -v "Input file\|Step\|Output\|Optimize level"
if [ -z "$(diff testcase/lexer/withfloat.lexer output3.txt 2>&1)" ]; then
    echo "PASS: withfloat.sy"
else
    echo "FAIL: withfloat.sy"
    echo "Differences:"
    diff testcase/lexer/withfloat.lexer output3.txt | head -10
fi
echo ""

echo "========================================"
echo "All tests completed"
echo "========================================"

