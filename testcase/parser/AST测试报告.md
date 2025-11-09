# AST测试报告

## 测试时间
2025-11-09

## 测试样例

### 1. simple.sy ✓ PASS
- **测试文件**: `testcase/parser/simple.sy`
- **生成的AST**: `testcase/parser/simple.ast`
- **标准AST**: `testcase/parser/simple.parser`
- **结果**: ✅ **完全一致**
- **说明**: 生成的AST与标准AST完全匹配，语法分析器正确处理了函数声明、变量声明、if语句、while循环、函数调用等基本语法结构。

### 2. witharray.sy ⚠️ 差异
- **测试文件**: `testcase/parser/witharray.sy`
- **生成的AST**: `testcase/parser/witharray.ast`
- **标准AST**: `testcase/parser/witharray.parser`
- **结果**: ⚠️ **存在差异**

#### 差异分析：
1. **标准AST中包含一个 `max` 函数**（第43-89行），但源代码 `witharray.sy` 中**没有这个函数**。
2. **标准AST的return语句**是：
   ```
   `-- ReturnStmt
       `-- BinaryExpr +
           |-- Call sum
           |   |-- Arg 0: 
           |   |   `-- LeftValueExpr arr
           |   `-- Arg 1: 
           |       `-- literal int: 100
           `-- Call max
               |-- Arg 0: 
               |   `-- LeftValueExpr arr
               |       `-- literal int: 0
               `-- Arg 1: 
                   `-- literal int: 10
   ```
3. **生成的AST的return语句**是：
   ```
   `-- ReturnStmt
       `-- Call sum
           |-- Arg 0: 
           |   `-- LeftValueExpr arr
           `-- Arg 1: 
               `-- literal int: 100
   ```

#### 结论：
- **生成的AST是正确的**，它准确反映了源代码 `witharray.sy` 的内容。
- **标准AST文件 `witharray.parser` 可能存在问题**，它包含了一个源代码中不存在的 `max` 函数，以及一个不存在的 `BinaryExpr +` 表达式。

### 3. withfloat.sy ✓ PASS
- **测试文件**: `testcase/parser/withfloat.sy`
- **生成的AST**: `testcase/parser/withfloat.ast`
- **标准AST**: `testcase/parser/withfloat.parser`
- **结果**: ✅ **完全一致**
- **说明**: 生成的AST与标准AST完全匹配，语法分析器正确处理了浮点数常量、十六进制浮点数、函数调用、if-else语句等语法结构。

## 总结

| 测试样例 | 结果 | 说明 |
|---------|------|------|
| simple.sy | ✅ PASS | 生成的AST与标准AST完全一致 |
| witharray.sy | ⚠️ 差异 | 生成的AST正确，但标准AST文件可能有问题（包含源代码中不存在的max函数） |
| withfloat.sy | ✅ PASS | 生成的AST与标准AST完全一致 |

## 建议

1. **simple.sy** 和 **withfloat.sy** 测试通过，说明语法分析器功能正常。
2. **witharray.sy** 的差异可能是标准AST文件 `witharray.parser` 与源代码 `witharray.sy` 不匹配导致的。建议：
   - 检查标准AST文件是否与源代码同步
   - 或者检查源代码文件是否被修改过
   - 生成的AST准确反映了源代码内容，语法分析器工作正常

## 生成的AST文件位置

- `testcase/parser/simple.ast`
- `testcase/parser/witharray.ast`
- `testcase/parser/withfloat.ast`

