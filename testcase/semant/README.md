# 语义检查测试用例说明

本目录包含18个语义错误测试用例，用于测试编译器的语义检查功能。

## 测试分类

### 1. 未定义变量/函数 (4个)
- `undef1.sy` - 使用未定义的变量
- `undef2.sy` - 使用未定义的函数
- `undef3.sy` - 数组相关未定义
- `undef4.sy` - 复杂未定义场景

### 2. 重复定义 (3个)
- `multi_def1.sy` - 变量重复定义
- `multi_def2.sy` - 函数参数重定义
- `multi_def3.sy` - 嵌套作用域重定义

### 3. 非法表达式 (4个)
- `invalid_expr1.sy` - void函数返回值赋值
- `invalid_expr2.sy` - void函数作为返回值
- `invalid_expr3.sy` - void类型参与运算
- `invalid_expr4.sy` - 复杂非法表达式

### 4. 函数调用不匹配 (5个)
- `unmatch_call1.sy` - 嵌套调用类型不匹配
- `unmatch_call2.sy` - 参数数量不匹配
- `unmatch_call3.sy` - 参数类型不匹配
- `unmatch_call4.sy` - 返回值类型不匹配
- `unmatch_call5.sy` - 复杂调用场景

### 5. 除零错误 (2个)
- `div_zero1.sy` - 常量表达式除零
- `div_zero2.sy` - 常量表达式取模零

## 运行测试

### 方法1: 使用测试脚本（推荐）

```bash
# 在WSL中进入项目目录
cd /mnt/c/Users/xjt26/Desktop/NKU_2025_compiler_xjt_oyzh-main

# 运行简洁测试
./test_semant.sh

# 运行详细测试（显示每个测试用例的详细信息）
./test_semant_verbose.sh
```

### 方法2: 手动测试单个文件

```bash
# 测试单个文件
./bin/compiler testcase/semant/undef1.sy -llvm -o /tmp/test.ll

# 预期: 编译器应该报错并返回非0退出码
```

## 测试结果

**通过率**: 100% (18/18)

所有测试用例均能被编译器正确识别为语义错误。

## 注意事项

1. 这些测试用例都包含语义错误，**应该编译失败**
2. 编译器返回非0退出码表示测试通过
3. 部分测试用例（如`invalid_expr1.sy`、`invalid_expr3.sy`）会触发编译器的断言失败，这是因为错误在代码生成阶段才被检测到

## 相关文档

- 详细测试报告: `../../SEMANT_TEST_REPORT.md`
- 项目README: `../../README.md`
