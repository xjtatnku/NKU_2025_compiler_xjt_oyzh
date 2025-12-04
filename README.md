# NKU-Compiler2025 - SysY 编译器实现

> **课程项目**：南开大学 2025 年秋季编译系统原理  
> **作者**：oyzh, xjt  
> **仓库地址**：[xjtatnku/NKU_2025_compiler_xjt_oyzh](https://github.com/xjtatnku/NKU_2025_compiler_xjt_oyzh)

## 📋 项目概述

完整实现 SysY 语言（类 C 语言子集）的编译器，包含以下功能：
- ✅ **词法分析** (Lab1) - 从源代码生成 Token 流
- ✅ **语法分析** (Lab2) - 使用 Bison/Yacc 构建抽象语法树（AST）
- ✅ **语义分析** (Lab3-1) - 类型检查和符号表管理
- ✅ **中间代码生成** (Lab3-2) - 从 AST 生成 LLVM IR

## 🎯 当前状态

### 已完成功能
- ✅ 完整的词法和语法分析
- ✅ 完整的语义检查和符号表
- ✅ 基本的 LLVM IR 代码生成
- ✅ 整数和浮点数类型支持
- ✅ 控制流结构（if/while/for/break/continue）
- ✅ 函数声明和调用
- ✅ 数组声明和访问
- ✅ 类型转换和推导

### 中间代码生成 Advanced 测试问题

**测试结果**：Advanced 测试通过率 **100/100 (100%)**

**核心问题**：**数组初始化列表的 IR 代码生成功能**
包括：
- 所有包含数组初始化列表的测试
- 多维数组初始化测试
- 部分初始化测试（如 `int a[10] = {1, 2};`）
- 复杂嵌套初始化列表测试

## 🚀 快速开始

### 环境要求
- 支持 C++17 的编译器（clang++ 或 g++）
- Flex/Bison 用于解析器生成
- LLVM 工具链用于 IR 编译
- Python 3 用于测试脚本

### 编译构建

```bash
# 清理并重新编译
make clean
make -j8  # 使用 8 个并行任务

# 或使用并行编译目标
make build-parallel
```

### 使用方法

```bash
# 将 SysY 源文件编译为 LLVM IR
./bin/compiler input.sy -llvm -o output.ll -O0

# 编译为汇编代码（待实现）
./bin/compiler input.sy -S -o output.s -O0

# 仅语法分析（生成 AST）
./bin/compiler input.sy -parser -o output.ast
```

## 🧪 测试

### 运行基础测试
```bash
python3 test.py --group Basic --stage llvm --opt 0
```

### 运行进阶测试（数组和浮点数）
```bash
python3 test.py --group Advanced --stage llvm --opt 0
```

### 测试结果
| 测试组 | 状态 | 通过率 | 备注 |
|--------|------|--------|------|
| **Basic** | ✅ | 100/100 | 基础语言特性 |
| **Advanced** | ✅ | 100/100 (100%) | 数组初始化已完成 |


## 📁 项目结构

```
NKU-Compiler2025-main/
├── frontend/                    # 前端：词法、语法、语义分析
│   ├── parser/                  # 解析器实现（Flex/Bison）
│   │   ├── lexer.l              # 词法分析规则
│   │   ├── yacc.y               # 语法分析规则（文法）
│   │   └── parser.cpp/h         # 解析器接口
│   ├── ast/                     # 抽象语法树定义
│   │   ├── expr.cpp/h           # 表达式节点
│   │   ├── stmt.cpp/h           # 语句节点
│   │   ├── decl.cpp/h           # 声明节点
│   │   └── visitor/             # 访问者模式实现
│   │       └── sementic_check/  # 语义分析访问者
│   └── symbol/                  # 符号表管理
│       └── symbol_table.cpp/h   # 符号表实现
│
├── middleend/                   # 中端：IR 生成与优化
│   ├── module/                  # IR 模块结构
│   │   ├── ir_module.cpp/h      # 模块容器
│   │   ├── ir_function.cpp/h    # 函数 IR
│   │   ├── ir_block.cpp/h       # 基本块
│   │   └── ir_instruction.cpp/h # 指令
│   ├── visitor/                 # IR 生成访问者
│   │   ├── codegen/             # AST 到 IR 代码生成
│   │   │   ├── ast_codegen.cpp/h
│   │   │   ├── expr_codegen.cpp
│   │   │   ├── stmt_codegen.cpp
│   │   │   ├── decl_codegen.cpp # 数组初始化完成
│   │   │   └── type_convert.cpp
│   │   └── printer/             # IR 美化输出
│   └── pass/                    # 优化遍
│       └── analysis/            # 分析遍（CFG、支配树等）
│
├── backend/                     # 后端：代码生成（待实现）
├── interfaces/                  # 公共接口和定义
├── utils/                       # 工具函数
├── lib/                         # 运行时库
│   ├── libsysy_x86.a           # x86 运行时
│   └── libsysy_riscv.a         # RISC-V 运行时
│
├── testcase/                    # 测试用例
│   └── functional/
│       ├── Basic/               # 基础功能测试
│       └── Advanced/            # 进阶测试（数组、浮点数）
│
├── test.py                      # 测试运行脚本
├── Makefile                     # 构建配置
├── README.md                    # 本文件
└── ADVANCED_TEST_REPORT.md      # 详细测试分析报告
```

## 🎨 支持的语言特性

编译器支持以下 SysY 语言特性：

### 数据类型
- `int` - 32 位整数
- `float` - 32 位浮点数
- 数组 - 多维 int/float 数组

### 声明语句
```c
int a = 10;                          // 带初始化的变量
const int N = 100;                   // 常量
int arr[10];                         // 数组
int matrix[3][4] = {{1,2,3,4},...}; // 带初始化列表的数组
float f = 3.14;                      // 浮点变量
```

### 表达式
- 算术运算：`+`、`-`、`*`、`/`、`%`
- 关系运算：`<`、`<=`、`>`、`>=`、`==`、`!=`
- 逻辑运算：`&&`、`||`、`!`
- 赋值运算：`=`

### 控制流
```c
if (cond) stmt;                      // 条件语句
if (cond) stmt1 else stmt2;          // if-else 语句
while (cond) stmt;                   // while 循环
for (init; cond; update) stmt;       // for 循环（语法糖）
break;                               // break 语句
continue;                            // continue 语句
return expr;                         // return 语句
```

### 函数
```c
int func(int a, float b) {           // 函数定义
    return a + b;
}

int main() {                         // 程序入口
    return func(1, 2.0);
}
```

## 🛠 技术栈

| 组件 | 技术 |
|------|------|
| **词法分析器** | Flex |
| **语法分析器** | Bison (LALR) |
| **AST** | C++ 访问者模式 |
| **语义分析** | 符号表 + 类型系统 |
| **中间表示** | LLVM IR |
| **后端** | LLVM (x86_64，RISC-V 计划中) |
| **构建系统** | GNU Make |
| **测试** | Python 3 + diff |

## 📚 文档

- **[ADVANCED_TEST_REPORT.md](ADVANCED_TEST_REPORT.md)** - Advanced 测试详细分析报告
- **前端 AST** - 参见 `frontend/ast/*.h` 了解节点定义
- **IR 指令** - 参见 `interfaces/middleend/ir_defs.h`
- **符号表** - 参见 `frontend/symbol/symbol_table.h`

## 👥 贡献者

- **oyzh** - 主要开发负责人
- - **xjt** - 优化测试维护

## 📄 许可证

本项目用于教学目的，是南开大学 2025 年编译系统原理课程的一部分。

## 🔗 参考资料

- [SysY 语言规范](https://gitlab.eduxiji.net/nscscc/compiler2024)
- [LLVM 文档](https://llvm.org/docs/)
- [Bison 手册](https://www.gnu.org/software/bison/manual/)

---

**最后更新**：2025 年 12 月 2 日  
**仓库地址**：https://github.com/xjtatnku/NKU_2025_compiler_xjt_oyzh
