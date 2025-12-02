#include <frontend/ast/visitor/sementic_check/ast_checker.h>
#include <debug.h>

namespace FE::AST
{
    bool ASTChecker::visit(ExprStmt& node)
    {
        // 示例实现：表达式语句的语义检查
        // 空表达式直接通过，否则访问内部表达式
        if (!node.expr) return true;
        return apply(*this, *node.expr);
    }

    bool ASTChecker::visit(FuncDeclStmt& node)
    {
        // TODO(Lab3-1): 实现函数声明的语义检查
        // 检查作用域，记录函数信息，处理形参和函数体，检查返回语句
        
        // 1. 检查函数是否重复定义
        // 全局查找：funcDecls 和 glbSymbols
        if (funcDecls.find(node.entry) != funcDecls.end() || glbSymbols.find(node.entry) != glbSymbols.end()) {
            errors.push_back("Function " + node.entry->getName() + " is already defined at line " + std::to_string(node.line_num));
            return false;
        }

        // 2. 注册函数
        funcDecls[node.entry] = &node;
        
        // 3. 检查是否是 main 函数
        if (node.entry->getName() == "main") {
            mainExists = true;
            // main 函数必须返回 int 且无参数 (SysY定义)
            // 但这里只需要检查 main 存在即可，严格的类型检查可视情况而定
            if (node.retType->getBaseType() != Type_t::INT) {
                 errors.push_back("main function must return int");
            }
            if (node.params && !node.params->empty()) {
                 errors.push_back("main function must not have parameters");
            }
        }

        // 4. 设置当前函数环境
        curFuncRetType = node.retType;
        funcHasReturn = false;

        // 5. 进入函数作用域 (用于存放参数)
        symTable.enterScope();

        // 6. 处理形参
        if (node.params) {
            for (auto* param : *node.params) {
                apply(*this, *param);
            }
        }

        // 7. 处理函数体
        if (node.body) {
            apply(*this, *node.body);
        }

        // 8. 退出作用域
        symTable.exitScope();

        // 9. 检查返回值 (Warning or Error?)
        // 如果是非 void 函数且没有 return 语句，通常是一个警告或错误
        // SysY spec implies main return 0 if missing, but for others it might be UB.
        // 这里我们不做强制报错，除非严格模式。
        
        return errors.empty();
    }

    bool ASTChecker::visit(VarDeclStmt& node)
    {
        // TODO(Lab3-1): 实现变量声明语句的语义检查
        // 空声明直接通过，否则委托给变量声明处理
        if (node.decl) {
            return apply(*this, *node.decl);
        }
        return true;
    }

    bool ASTChecker::visit(BlockStmt& node)
    {
        // TODO(Lab3-1): 实现块语句的语义检查
        // 进入新作用域，逐条访问语句，退出作用域
        symTable.enterScope();

        if (node.stmts) {
            for (auto* stmt : *node.stmts) {
                if (stmt) apply(*this, *stmt);
            }
        }

        symTable.exitScope();
        return true;
    }

    bool ASTChecker::visit(ReturnStmt& node)
    {
        // TODO(Lab3-1): 实现返回语句的语义检查
        // 设置返回标记，检查作用域，检查返回值类型匹配
        funcHasReturn = true;

        Type* actualRetType = voidType;
        if (node.retExpr) {
            apply(*this, *node.retExpr);
            actualRetType = node.retExpr->attr.val.value.type;
        }

        // 检查返回类型匹配
        // void 函数不能有返回值 (除了 empty return)
        if (curFuncRetType->getBaseType() == Type_t::VOID) {
            if (actualRetType->getBaseType() != Type_t::VOID) {
                errors.push_back("Void function cannot return a value at line " + std::to_string(node.line_num));
                return false;
            }
        } else {
            // 非 void 函数必须返回匹配的类型 (或可隐式转换)
            if (actualRetType->getBaseType() == Type_t::VOID) {
                 errors.push_back("Non-void function must return a value at line " + std::to_string(node.line_num));
                 return false;
            }
            // 这里的类型检查可以更严格，但 int/float 通常可以互转
        }
        
        return true;
    }

    bool ASTChecker::visit(WhileStmt& node)
    {
        // TODO(Lab3-1): 实现while循环的语义检查
        // 检查作用域，访问条件表达式，管理循环深度，访问循环体
        
        if (node.cond) {
            apply(*this, *node.cond);
            // 条件必须是 int/bool/float (scalar)
            if (node.cond->attr.val.value.type->getBaseType() == Type_t::VOID) {
                errors.push_back("Condition cannot be void at line " + std::to_string(node.line_num));
            }
        }

        loopDepth++;
        if (node.body) {
            apply(*this, *node.body); // Body usually is a block or stmt
            // Note: Body might be a BlockStmt, which handles its own scope. 
            // If body is a single statement, it shares scope? 
            // In C/SysY, `while(1) int a;` is valid only if it's a block? 
            // Actually `while(1) stmt` -> if stmt is decl, it must be in block in C89, but C99 allows it restrictedly.
            // SysY spec: Stmt -> Block | ... | Decl.
            // If Decl is a direct child of While, it might be scoped to the loop body.
            // Typically parser wraps body in Block if needed, or we rely on BlockStmt check.
        }
        loopDepth--;
        return true;
    }

    bool ASTChecker::visit(IfStmt& node)
    {
        // TODO(Lab3-1): 实现if语句的语义检查
        // 检查作用域，访问条件表达式，分别访问then和else分支
        
        if (node.cond) {
            apply(*this, *node.cond);
             if (node.cond->attr.val.value.type->getBaseType() == Type_t::VOID) {
                errors.push_back("Condition cannot be void at line " + std::to_string(node.line_num));
            }
        }

        if (node.thenStmt) apply(*this, *node.thenStmt);
        if (node.elseStmt) apply(*this, *node.elseStmt);
        
        return true;
    }

    bool ASTChecker::visit(BreakStmt& node)
    {
        // TODO(Lab3-1): 实现break语句的语义检查
        // 检查是否在循环内使用
        if (loopDepth == 0) {
            errors.push_back("Break statement not within loop at line " + std::to_string(node.line_num));
            return false;
        }
        return true;
    }

    bool ASTChecker::visit(ContinueStmt& node)
    {
        // TODO(Lab3-1): 实现continue语句的语义检查
        // 检查是否在循环内使用
        if (loopDepth == 0) {
            errors.push_back("Continue statement not within loop at line " + std::to_string(node.line_num));
            return false;
        }
        return true;
    }

    bool ASTChecker::visit(ForStmt& node)
    {
        // TODO(Lab3-1): 实现for循环的语义检查
        // 检查作用域，访问初始化、条件、步进表达式，管理循环深度
        
        // For loop creates a scope for init-statement (if it's a decl)
        symTable.enterScope();

        if (node.init) apply(*this, *node.init);
        if (node.cond) {
            apply(*this, *node.cond);
             if (node.cond->attr.val.value.type->getBaseType() == Type_t::VOID) {
                errors.push_back("Condition cannot be void at line " + std::to_string(node.line_num));
            }
        }
        if (node.step) apply(*this, *node.step);

        loopDepth++;
        if (node.body) apply(*this, *node.body);
        loopDepth--;

        symTable.exitScope();
        return true;
    }
}  // namespace FE::AST
