# **NKU-Compilers2025**



学术诚信问题



在基于本框架完成实验时，可以参考开源仓库中的代码，但必须在注释中明确注明自己哪部分代码是参考的其他仓库，并给出自己参考仓库的具体链接，文件以及哪一行代码。如果参考了公开源代码，但是没有在注释中明确注明，将按抄袭处理，当此作业记0分，下面是一个注释的例子。



// Reference: https://github.com/llvm/llvm-project/blob/main/llvm/lib/Transforms/Scalar/LICM.cpp line1-line37



## Lab1：词法分析器

1. 在yacc.y中补充缺失的token定义（运算符、类型关键字等）
2. 在lexer.l中实现运算符的正则表达式和处理逻辑
3. 在lexer.l中实现浮点数（十进制和十六进制）的处理
4. 在lexer.l中实现多行注释的处理
5. 更新parser.cpp以支持FLOAT_CONST和LL_CONST
6. 测试词法分析器是否正常工作
7. 添加字符串转义字符处理和错误报告功能
8. 添加详细的中文注释和说明
9. 测试样例结果输出与保存
10. git工程维护

Reference:[CentaureaHO/NKU-Compiler2025: 南开大学2025年编译系统原理实验代码框架](https://github.com/CentaureaHO/NKU-Compiler2025)

**需要阅读并编写的代码：**

- **frontend/parser/yacc.y: 补充你需要使用的 token 声明**
- **frontend/parser/lexer.l : 编写你想实现的词法正则表达式及对应处理函数**

以上为参考文件

## 第一步：进入虚拟机

```bash
wsl -d Ubuntu-22.04
```

## 第二步：进入项目目录

```bash
cd /mnt/c/Users/xjt26/Desktop/NKU-Compiler2025-main
```

## 第三步：运行演示脚本

```bash
bash demo.sh
```

## 第四步：观察输出

```bash
   # 查看期望输出
   cat testcase/lexer/simple.lexer

   # 显示差异
   diff testcase/lexer/simple.lexer output1.txt

   # 统计token数量
   wc -l output1.txt
   grep -c "^TOKEN" output1.txt 
```

