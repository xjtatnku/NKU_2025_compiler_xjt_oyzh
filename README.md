# NKU-Compilers2025 Lab2: 语法分析器

## 实验目的

本实验的目标是实现语法分析器（Parser），将词法分析器生成的 Token 序列转换为抽象语法树（Abstract Syntax Tree, AST）。

通过本实验，你将：
- 理解语法分析的基本原理和 LALR(1) 语法分析方法
- 掌握使用 Bison/Yacc 工具定义语法规则
- 学会构建和维护抽象语法树（AST）
- 理解运算符优先级和结合性的处理
- 掌握语法冲突的识别和解决方法

## 实验内容

### 1. 语法规则定义

在 `frontend/parser/yacc.y` 中定义 SysY 语言的语法规则，包括：

- **程序结构**：程序由语句列表组成
- **声明语句**：变量声明、函数声明
- **表达式**：算术表达式、逻辑表达式、关系表达式、赋值表达式、函数调用表达式
- **控制流语句**：if-else、while、for、break、continue、return
- **复合语句**：代码块（Block）

### 2. AST 节点构建

在语法规则的动作部分构建相应的 AST 节点：

- **表达式节点**：字面量表达式、左值表达式、一元表达式、二元表达式、函数调用表达式
- **语句节点**：表达式语句、变量声明语句、函数声明语句、控制流语句、返回语句
- **声明节点**：变量声明、参数声明、初始化列表

### 3. 运算符优先级和结合性

通过 `%left`、`%right`、`%precedence` 声明明确运算符的优先级和结合性：

```yacc
%left COMMA                    // 逗号运算符，左结合
%right ASSIGN                  // 赋值运算符，右结合
%left OR                       // 逻辑或，左结合
%left AND                      // 逻辑与，左结合
%left EQ NE                    // 相等性比较，左结合
%left LT LE GT GE              // 关系比较，左结合
%left PLUS MINUS               // 加减运算，左结合
%left STAR SLASH MOD           // 乘除模运算，左结合
```

### 4. 语法冲突处理

- **移进-归约冲突**：通过优先级声明解决运算符歧义
- **归约-归约冲突**：通过调整语法规则结构解决
- **悬空else问题**：使用 `%precedence THEN` 和 `%precedence ELSE` 解决

## 实现的功能

### 基本功能

✅ **数据类型支持**
- `int`：整数类型
- `float`：浮点数类型（包括十进制和十六进制浮点数）

✅ **变量声明**
- 基本变量声明：`int a;`
- 变量初始化：`int a = 10;`
- 多变量声明：`int a, b, c;`
- 数组声明：`int arr[10];`、`int arr[10][10];`

✅ **函数声明和定义**
- 函数声明：`int add(int a, int b);`
- 函数定义：包含函数体（代码块）
- 函数参数：支持多个参数，包括数组参数

✅ **表达式**
- **算术表达式**：`+`、`-`、`*`、`/`、`%`
- **关系表达式**：`<`、`<=`、`>`、`>=`
- **相等性表达式**：`==`、`!=`
- **逻辑表达式**：`&&`、`||`、`!`
- **赋值表达式**：`=`
- **一元表达式**：`+`、`-`、`!`
- **函数调用表达式**：`func(arg1, arg2)`
- **数组访问表达式**：`arr[i]`、`arr[i][j]`
- **字面量表达式**：整数、长整数、浮点数

✅ **控制流语句**
- **if-else语句**：`if (condition) stmt;`、`if (condition) stmt1 else stmt2;`
- **while循环**：`while (condition) stmt;`
- **for循环**：`for (init; condition; update) stmt;`
- **break语句**：`break;`
- **continue语句**：`continue;`
- **return语句**：`return;`、`return expr;`

✅ **复合语句**
- **代码块**：`{ stmt1; stmt2; ... }`
- **空代码块**：`{}`

### 高级特性

✅ **运算符优先级和结合性**
- 正确实现了所有运算符的优先级关系
- 赋值运算符右结合，其他运算符左结合

✅ **语法错误处理**
- 使用 Bison 的错误恢复机制
- 提供详细的错误位置信息（行号和列号）

✅ **AST 结构一致性**
- 函数体统一返回 `BlockStmt` 类型
- 左值表达式正确标记 `isLval` 属性

## 项目结构

```
lab2-main/
├── frontend/
│   ├── parser/
│   │   ├── yacc.y          # 语法规则定义（主要实现文件）
│   │   ├── lexer.l          # 词法规则定义（Lab1完成）
│   │   ├── parser.h/cpp     # 解析器接口实现
│   │   └── scanner.h        # 词法扫描器
│   ├── ast/                 # AST节点定义
│   │   ├── ast.h/cpp        # AST基类
│   │   ├── expr.h/cpp        # 表达式节点
│   │   ├── stmt.h/cpp        # 语句节点
│   │   ├── decl.h/cpp        # 声明节点
│   │   └── visitor/          # 访问者模式实现
│   └── symbol/               # 符号表（Lab3使用）
├── testcase/
│   └── parser/               # 语法分析测试用例
│       ├── simple.sy         # 基础语法测试
│       ├── simple.parser     # 标准AST输出
│       ├── witharray.sy      # 数组语法测试
│       ├── witharray.parser  # 标准AST输出
│       ├── withfloat.sy      # 浮点数语法测试
│       └── withfloat.parser  # 标准AST输出
├── Makefile                  # 构建配置
└── README.md                 # 本文件
```

## 编译和运行

### 1. 编译

```bash
# 默认串行编译
make

# 并行编译（推荐，大幅提升编译速度）
make build-parallel

# 或手动指定并行任务数
make -j16  # 根据CPU核心数调整

# 查看编译信息
make info

# 清除编译产物
make clean
```

### 2. 运行语法分析器

```bash
# 语法分析，生成AST并输出到文件
./bin/compiler -parser -o output.ast input.sy

# 示例：测试simple.sy
./bin/compiler -parser -o testcase/parser/simple.ast testcase/parser/simple.sy
```

### 3. 测试用例

项目提供了三个测试用例：

- **simple.sy**：测试基本语法结构（函数、变量、if、while、函数调用）
- **witharray.sy**：测试数组声明和访问
- **withfloat.sy**：测试浮点数常量和运算

运行测试：
```bash
# 测试simple.sy
./bin/compiler -parser -o testcase/parser/simple.ast testcase/parser/simple.sy
diff testcase/parser/simple.parser testcase/parser/simple.ast

# 测试witharray.sy
./bin/compiler -parser -o testcase/parser/witharray.ast testcase/parser/witharray.sy
diff testcase/parser/witharray.parser testcase/parser/witharray.ast

# 测试withfloat.sy
./bin/compiler -parser -o testcase/parser/withfloat.ast testcase/parser/withfloat.sy
diff testcase/parser/withfloat.parser testcase/parser/withfloat.ast
```

## 测试结果

| 测试用例 | 结果 | 说明 |
|---------|------|------|
| simple.sy | ✅ PASS | 生成的AST与标准AST完全一致 |
| witharray.sy | ✅ PASS | 正确解析数组声明和访问 |
| withfloat.sy | ✅ PASS | 正确解析浮点数字面量和运算 |

详细测试报告请参考：`testcase/parser/AST测试报告.md`

## 关键技术点

### 1. 运算符优先级处理

通过 Bison 的优先级声明机制，明确指定运算符的优先级和结合性，避免语法歧义：

```yacc
%left PLUS MINUS               // 加减运算，左结合
%left STAR SLASH MOD           // 乘除模运算，左结合（优先级高于加减）
```

### 2. 函数体统一处理

确保函数体始终返回 `BlockStmt` 类型，即使函数体为空或只有一条语句：

```yacc
FUNC_BODY:
    LBRACE RBRACE {
        $$ = new BlockStmt(new std::vector<StmtNode*>(), @1.begin.line, @1.begin.column);
    }
    | LBRACE STMT_LIST RBRACE {
        if (!$2 || $2->empty())
        {
            $$ = new BlockStmt(new std::vector<StmtNode*>(), @1.begin.line, @1.begin.column);
            delete $2;
        }
        else $$ = new BlockStmt($2, @1.begin.line, @1.begin.column);
    }
    ;
```

### 3. 左值表达式标记

在构建左值表达式时，明确设置 `isLval` 属性：

```yacc
LEFT_VAL_EXPR:
    IDENT {
        Entry* entry = Entry::getEntry($1);
        LeftValExpr* lval = new LeftValExpr(entry, nullptr, @1.begin.line, @1.begin.column);
        lval->isLval = true;  // 标识符作为左值
        $$ = lval;
    }
    ;
```

### 4. 悬空else问题解决

使用 `%precedence` 声明解决 if-else 的移进-归约冲突：

```yacc
%precedence THEN
%precedence ELSE
```

## 注意事项

1. **语法规则顺序**：Bison 使用 LALR(1) 算法，语法规则的顺序会影响冲突的解决
2. **内存管理**：在构建 AST 时注意内存分配和释放，避免内存泄漏
3. **错误处理**：语法错误时提供准确的行号和列号信息
4. **AST 一致性**：确保相同类型的语法结构生成相同类型的 AST 节点

## 参考资料

- Bison 官方文档：https://www.gnu.org/software/bison/manual/
- SysY 语言规范：`doc/SysY2022语言定义-V1.pdf`（如果存在）
- AST 节点定义：`frontend/ast/` 目录下的头文件

## 完成标准

完成本实验后，你的语法分析器应该能够：

1. ✅ 正确解析 SysY 语言的所有基本语法结构
2. ✅ 生成结构正确的 AST
3. ✅ 通过所有测试用例（simple.sy、witharray.sy、withfloat.sy）
4. ✅ 正确处理运算符优先级和结合性
5. ✅ 正确标记左值表达式
6. ✅ 提供准确的语法错误信息

Reference: https://github.com/CentaureaHO/NKU-Compiler2025
## 后续实验

完成 Lab2 后，可以继续：

- **Lab3-1：语义分析**：在 AST 基础上进行语义检查，构建符号表
- **Lab3-2：中间代码生成**：将 AST 转换为中间表示（IR）
