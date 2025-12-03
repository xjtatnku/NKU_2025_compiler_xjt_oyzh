#include <frontend/ast/visitor/sementic_check/ast_checker.h>
#include <debug.h>

namespace FE::AST
{
    bool ASTChecker::visit(LeftValExpr& node)
    {
        // TODO(Lab3-1): 实现左值表达式的语义检查
        // 检查变量是否存在，处理数组下标访问，进行类型检查和常量折叠
        
        // 1. 查找符号
        VarAttr* attr = symTable.getSymbol(node.entry);
        if (!attr) {
            // Try global map if not found (though symTable should handle it)
             auto it = glbSymbols.find(node.entry);
             if (it != glbSymbols.end()) {
                 attr = &it->second;
             } else {
                 errors.push_back("Undefined variable " + node.entry->getName() + " at line " + std::to_string(node.line_num));
                 return false;
             }
        }

        // 2. 处理下标
        size_t indexCount = node.indices ? node.indices->size() : 0;
        size_t dimCount = attr->arrayDims.size();

        if (indexCount > dimCount) {
             errors.push_back("Too many subscripts for array " + node.entry->getName() + " at line " + std::to_string(node.line_num));
             return false;
        }

        if (node.indices) {
            for (auto* index : *node.indices) {
                apply(*this, *index);
                // Index must be int
                if (index->attr.val.value.type->getBaseType() != Type_t::INT) {
                     // Warning or implicit cast? Array index should be int.
                     // SysY spec says index is int.
                }
            }
        }

        // 3. 确定类型
        if (indexCount == dimCount) {
            // Fully dereferenced -> Base type
            node.attr.val.value.type = attr->type;
        } else {
            // Partial dereferenced -> Pointer to subarray
            // int a[2][3]; a[0] -> int (*)[3]
            // For simplicity, we might just say it's a pointer type or array type
            node.attr.val.value.type = TypeFactory::getPtrType(attr->type);
        }

        // 4. 常量折叠
        if (attr->isConstDecl) {
            bool allIndicesConst = true;
            std::vector<int> indices;
            if (node.indices) {
                for (auto* index : *node.indices) {
                    if (!index->attr.val.isConstexpr) {
                        allIndicesConst = false;
                        break;
                    }
                    indices.push_back(index->attr.val.getInt());
                }
            }

            if (allIndicesConst) {
                node.attr.val.isConstexpr = true;
                // 如果是标量且有初始值
                if (dimCount == 0 && !attr->initList.empty()) {
                     // Use the declared type, not the init value's type
                     // This handles cases like: const int x = 1e9; where 1e9 is float but x is int
                     node.attr.val.value.type = attr->type;
                     
                     // Convert init value to the declared type
                     const VarValue& initVal = attr->initList[0];
                     if (attr->type->getBaseType() == Type_t::INT) {
                         node.attr.val.value.intValue = initVal.getInt();
                     } else if (attr->type->getBaseType() == Type_t::FLOAT) {
                         node.attr.val.value.floatValue = initVal.getFloat();
                     } else {
                         node.attr.val.value = initVal;
                     }
                } 
                // 如果是数组，需要根据 indices 计算偏移查找 initList
                // TODO: Implement array const value retrieval
                // Currently leaving it as just marking isConstexpr without value if complex
            }
        }
        
        node.isLval = (indexCount == dimCount); // Only fully dereferenced can be lval? 
        // Actually `a` (array name) is not lval in C assign sense, but `a[i]` is.
        // SysY: "LVal -> Ident {'[' Exp ']'}"
        
        return true;
    }

    bool ASTChecker::visit(LiteralExpr& node)
    {
        // 示例实现：字面量表达式的语义检查
        // 字面量总是编译期常量，直接设置属性
        node.attr.val.isConstexpr = true;
        node.attr.val.value       = node.literal;
        return true;
    }

    bool ASTChecker::visit(UnaryExpr& node)
    {
        // TODO(Lab3-1): 实现一元表达式的语义检查
        // 访问子表达式，检查操作数类型，调用类型推断函数
        if (node.expr) {
            apply(*this, *node.expr);
            bool hasError = false;
            node.attr.val = typeInfer(node.expr->attr.val, node.op, node, hasError);
        }
        return true;
    }

    bool ASTChecker::visit(BinaryExpr& node)
    {
        // TODO(Lab3-1): 实现二元表达式的语义检查
        // 访问左右子表达式，检查操作数类型，调用类型推断
        if (node.lhs && node.rhs) {
            apply(*this, *node.lhs);
            apply(*this, *node.rhs);
            bool hasError = false;
            node.attr.val = typeInfer(node.lhs->attr.val, node.rhs->attr.val, node.op, node, hasError);
        }
        return true;
    }

    bool ASTChecker::visit(CallExpr& node)
    {
        // TODO(Lab3-1): 实现函数调用表达式的语义检查
        // 检查函数是否存在，访问实参列表，检查参数数量和类型匹配
        
        // 1. 查找函数
        auto it = funcDecls.find(node.func);
        if (it == funcDecls.end()) {
             errors.push_back("Undefined function " + node.func->getName() + " at line " + std::to_string(node.line_num));
             node.attr.val.value.type = voidType;
             return false;
        }
        FuncDeclStmt* funcDecl = it->second;

        // 2. 检查参数
        size_t paramCount = funcDecl->params ? funcDecl->params->size() : 0;
        size_t argCount = node.args ? node.args->size() : 0;

        if (paramCount != argCount) {
             errors.push_back("Argument count mismatch for function " + node.func->getName() + 
                              ": expected " + std::to_string(paramCount) + ", got " + std::to_string(argCount) + 
                              " at line " + std::to_string(node.line_num));
        }

        if (node.args) {
            for (size_t i = 0; i < argCount; ++i) {
                ExprNode* arg = (*node.args)[i];
                apply(*this, *arg);
                
                // Check if argument is void (e.g., void function return as argument)
                if (arg->attr.val.value.type && arg->attr.val.value.type->getBaseType() == Type_t::VOID) {
                    errors.push_back("Argument " + std::to_string(i+1) + " cannot be void at line " + std::to_string(node.line_num));
                }

                if (i < paramCount) {
                    ParamDeclarator* param = (*funcDecl->params)[i];
                    Type* paramType = param->type; 
                    // 注意：ParamDeclarator 中的 type 只是 base type，如果 dims 存在，则是指针/数组
                    if (param->dims && !param->dims->empty()) {
                        paramType = TypeFactory::getPtrType(paramType);
                    }
                    
                    Type* argType = arg->attr.val.value.type;
                    
                    // Simple type check
                    // Allow int <-> float? allow int <-> bool?
                    // Pointers must match strictly or be compatible
                    // SysY spec implies basic types are compatible with implicit cast
                    
                    if (paramType->getTypeGroup() == TypeGroup::POINTER) {
                        if (argType->getTypeGroup() != TypeGroup::POINTER) {
                             errors.push_back("Argument " + std::to_string(i+1) + " type mismatch: expected pointer, got basic at line " + std::to_string(node.line_num));
                        }
                        // TODO: Check pointer base types
                    }
                }
            }
        }

        // 3. 设置返回值类型
        node.attr.val.value.type = funcDecl->retType;
        node.attr.val.isConstexpr = false; // Function calls are not constant in SysY? (Usually no)

        return true;
    }

    bool ASTChecker::visit(CommaExpr& node)
    {
        // TODO(Lab3-1): 实现逗号表达式的语义检查
        // 依序访问各子表达式，结果为最后一个表达式的属性
        if (!node.exprs || node.exprs->empty()) return true;

        for (auto* expr : *node.exprs) {
            apply(*this, *expr);
        }

        node.attr = node.exprs->back()->attr;
        return true;
    }
}  // namespace FE::AST
