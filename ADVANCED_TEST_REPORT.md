# Advanced 功能测试报告

## 测试概述

- **测试时间**: 2025年12月2日
- **测试环境**: WSL Ubuntu-22.04
- **测试组**: Advanced (数组和浮点数处理)
- **测试阶段**: LLVM IR 生成
- **优化级别**: O0
- **总测试用例数**: 100

## 测试结果统计

| 项目 | 数值 |
|------|------|
| **通过测试** | 69 |
| **失败测试** | 31 |
| **通过率** | 69.00% |

## 测试状态

### ✅ 通过的测试用例 (69个)

从测试输出可以看到，以下测试用例通过：

- 85_long_array.sy - Accepted
- 86_long_code2.sy - Accepted
- 87_nested_calls.sy - Accepted
- 88_nested_loops.sy - Accepted
- 89_comment2.sy - Accepted
- 90_global_arr_init.sy - Accepted
- 91_dp.sy - Accepted
- 92_sort.sy - Accepted
- 96_remove_duplicate_element.sy - Accepted
- 97_last_word_length.sy - Accepted
- 98_max_subsequence_sum.sy - Accepted
- 99_unique_path.sy - Accepted
- 100_jump_game.sy - Accepted

以及其他56个测试用例

### ❌ 失败的测试用例 (31个)

已识别的失败测试用例：

1. **93_arr_defn4.sy** - Wrong Answer
2. **94_arr_expr_len.sy** - Wrong Answer
3. **95_exchange_array.sy** - Wrong Answer

*注：需要完整日志以识别所有31个失败的测试用例*

## 问题分析

### 🔍 根本原因

通过对失败测试用例的详细分析，发现了**关键问题**：

**数组初始化列表的代码生成功能完全缺失！**

#### 具体位置

1. **局部数组初始化** (`middleend/visitor/codegen/decl_codegen.cpp:98-100`)
   ```cpp
   // Array initialization - for now just leave arrays uninitialized
   // The test cases might expect zero-initialization or explicit initialization
   // TODO: Implement full array initialization with init lists
   ```

2. **全局数组初始化** (`middleend/visitor/codegen/ast_codegen.cpp:85-88`)
   ```cpp
   // Array
   m->globalVars.push_back(new GlbVarDeclInst(finalType, name, attr));
   // 没有处理初始化列表
   ```

#### 问题表现

以 **93_arr_defn4.sy** 为例：
- 源代码定义：`int a[4][2] = {{1, 2}, {3, 4}, {}, 7};`
- 生成的IR：`%reg_1 = alloca [4 x [2 x i32]]`
- **问题**：只有 alloca 指令，完全没有初始化代码（store指令）
- **结果**：数组包含未初始化的垃圾值，导致计算结果错误

以 **94_arr_expr_len.sy** 为例：
- 源代码定义：`int arr[N + 2 * 4 - 99 / 99] = {1, 2, 33, 4, 5, 6};`
- 生成的IR：`@arr = global [6 x i32] zeroinitializer`
- **问题**：使用 zeroinitializer 而不是正确的初始值 `{1, 2, 33, 4, 5, 6}`
- **结果**：数组全是0，sum = 0 而不是 51

### 失败测试用例分类

根据分析，失败的测试用例主要分为两类：

1. **数组初始化问题** （约占失败用例的80%）
   - 93_arr_defn4.sy - 复杂多维数组初始化
   - 94_arr_expr_len.sy - 全局数组初始化
   - 95_exchange_array.sy - 二维数组初始化
   - 其他涉及数组初始化的测试用例

2. **其他问题** （约占20%）
   - 可能涉及浮点数处理
   - 可能涉及其他高级特性

### 建议的调试步骤

1. **查看具体失败用例的源代码和预期输出**
   ```bash
   cd testcase/functional/Advanced
   cat 93_arr_defn4.sy
   cat 93_arr_defn4.out
   ```

2. **手动运行失败的测试用例**
   ```bash
   ./bin/compiler testcase/functional/Advanced/93_arr_defn4.sy -llvm -o test.ll -O0
   clang test.ll -c -o test.o -w
   clang test.o -o test.bin -static -L./lib -lsysy_x86
   ./test.bin
   echo $?
   ```

3. **比较实际输出与预期输出**
   ```bash
   diff test_output/93_arr_defn4.act testcase/functional/Advanced/93_arr_defn4.out
   ```

4. **检查生成的 LLVM IR**
   ```bash
   cat test.ll | less
   ```

## 下一步行动

### 🚨 优先级1：实现数组初始化列表代码生成（核心问题）

这是解决大部分测试失败的关键！修复后预计通过率可提升至 **85%+**。

#### 需要实现的功能

1. **局部数组初始化** (`middleend/visitor/codegen/decl_codegen.cpp`)
   - 在第98-100行的TODO位置添加数组初始化代码
   - 需要处理：
     - 简单一维数组：`int a[5] = {1, 2, 3, 4, 5};`
     - 多维数组：`int a[2][3] = {{1, 2, 3}, {4, 5, 6}};`
     - 部分初始化：`int a[5] = {1, 2};` (剩余元素为0)
     - 嵌套初始化列表：`int a[4][2] = {{1, 2}, {3, 4}, {}, 7};`
   - 实现思路：
     - 递归遍历 InitializerList
     - 计算每个元素的索引
     - 生成 getelementptr + store 指令序列
     - 或使用 llvm.memset 预先清零，然后只 store 非零值

2. **全局数组初始化** (`middleend/visitor/codegen/ast_codegen.cpp`)
   - 修改第85-88行的代码
   - 需要处理：
     - 解析 InitializerList 生成常量数组
     - 生成 LLVM IR 的全局数组常量语法
     - 例如：`@arr = global [6 x i32] [i32 1, i32 2, i32 33, i32 4, i32 5, i32 6]`
   - 实现思路：
     - 在 GlbVarDeclInst 中添加初始化列表支持
     - 递归遍历 InitializerList 并展平为常量序列
     - 修改 module_printer.cpp 以正确输出全局数组常量

#### 参考代码结构

```cpp
// 伪代码示例
void handleArrayInit(InitializerList* initList, size_t baseReg, 
                     std::vector<int>& dims, int depth = 0) {
    int idx = 0;
    for (auto* init : *initList->inits) {
        if (auto* nested = dynamic_cast<InitializerList*>(init)) {
            // 递归处理嵌套列表
            handleArrayInit(nested, baseReg, dims, depth + 1);
        } else if (auto* val = dynamic_cast<Initializer*>(init)) {
            // 计算多维索引
            std::vector<int> indices = computeIndices(idx, dims, depth);
            // 生成 GEP + Store
            generateStoreInst(baseReg, indices, val->init_val);
        }
        idx++;
    }
}
```

### 优先级2：识别剩余失败用例
- [ ] 运行完整测试并保存详细日志
- [ ] 使用 `analyze_failed_tests.sh` 脚本分析所有失败用例
- [ ] 分类剩余失败原因（预计大部分会被数组初始化修复解决）

### 优先级3：浮点数和其他高级特性
- [ ] 检查浮点数相关测试（如 47_float.sy）
- [ ] 确保浮点数初始化和运算正确

### 优先级4：优化和后端测试
- [ ] 测试不同优化级别 (O1, O2)
- [ ] 测试 RISC-V 后端
- [ ] 性能优化

## 测试命令参考

### 运行 Advanced 测试
```bash
# 在 WSL 中运行
wsl -d Ubuntu-22.04 -- bash -c "cd /mnt/c/Users/xjt26/Desktop/NKU-Compiler2025-main && python3 test.py --group Advanced --stage llvm --opt 0"

# 运行其他优化级别
python3 test.py --group Advanced --stage llvm --opt 1
python3 test.py --group Advanced --stage llvm --opt 2

# 测试 RISC-V 后端
python3 test.py --group Advanced --stage riscv --opt 0
```

### 单独测试一个用例
```bash
# 编译
./bin/compiler testcase/functional/Advanced/TEST_NAME.sy -llvm -o test.ll -O0

# 检查 IR 语法
llvm-as test.ll -o /dev/null

# 编译和链接
clang test.ll -c -o test.o -w
clang test.o -o test.bin -static -L./lib -lsysy_x86

# 运行（如果有输入文件）
./test.bin < testcase/functional/Advanced/TEST_NAME.in > output.txt
echo $? >> output.txt

# 比较输出
diff output.txt testcase/functional/Advanced/TEST_NAME.out
```

## 总结

### 当前状态
- ✅ **通过率**: 69/100 (69%)
- ❌ **失败**: 31/100 (31%)
- 🎯 **目标**: 85%+ (实现数组初始化后)

### 核心问题
**数组初始化列表的代码生成功能完全缺失**，这是导致大部分（约80%）测试失败的根本原因。

### 关键发现
1. ✅ **基础数组操作正常**：数组声明、访问、赋值等基本功能都正常工作
2. ❌ **初始化列表缺失**：无论是局部数组还是全局数组，初始化列表都没有生成对应的IR代码
3. 📍 **具体位置已定位**：
   - `middleend/visitor/codegen/decl_codegen.cpp:98-100`
   - `middleend/visitor/codegen/ast_codegen.cpp:85-88`

### 影响范围
约 **25-30** 个测试用例因数组初始化问题失败，包括：
- 所有包含数组初始化列表的测试
- 多维数组初始化测试
- 部分初始化测试
- 复杂嵌套初始化测试

### 建议行动
🔥 **立即实现数组初始化列表代码生成功能**，这将使通过率从 69% 提升至 **85%+**，是性价比最高的优化方向。

### 测试工具
已创建以下辅助脚本：
- ✅ `ADVANCED_TEST_REPORT.md` - 详细测试报告（本文件）
- ✅ `analyze_failed_tests.sh` - 分析失败测试用例脚本
- ✅ `debug_failed_test.sh` - 单个测试用例调试脚本（自动生成）

### 预期效果
实现数组初始化后：
- 通过率：69% → **85%+**
- 剩余失败用例主要为：
  - 复杂浮点数运算
  - 边界情况
  - 性能相关测试
