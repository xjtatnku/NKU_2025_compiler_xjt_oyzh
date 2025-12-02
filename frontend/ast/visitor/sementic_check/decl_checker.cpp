#include <frontend/ast/visitor/sementic_check/ast_checker.h>
#include <debug.h>

namespace FE::AST
{
    bool ASTChecker::visit(Initializer& node)
    {
        // 示例实现：单个初始化器的语义检查
        // 1) 访问初始化值表达式
        // 2) 将子表达式的属性拷贝到当前节点
        ASSERT(node.init_val && "Null initializer value");
        bool res  = apply(*this, *node.init_val);
        node.attr = node.init_val->attr;
        return res;
    }

    bool ASTChecker::visit(InitializerList& node)
    {
        // 示例实现：初始化器列表的语义检查
        // 遍历列表中的每个初始化器并逐个访问
        if (!node.init_list) return true;
        bool res = true;
        for (auto* init : *(node.init_list))
        {
            if (!init) continue;
            res &= apply(*this, *init);
        }
        return res;
    }

    bool ASTChecker::visit(VarDeclarator& node)
    {
        // TODO(Lab3-1): 实现变量声明器的语义检查
        // 访问左值表达式，同步属性，处理初始化器（如果有）
        // Logic handled in VarDeclaration to access type info
        (void)node;
        return true;
    }

    bool ASTChecker::visit(ParamDeclarator& node)
    {
        // TODO(Lab3-1): 实现函数形参的语义检查
        // 检查形参重定义，处理数组形参的类型退化，将形参加入符号表
        
        // 1. 检查重定义
        VarAttr* existing = symTable.getSymbol(node.entry);
        // 此时还在函数参数作用域（与函数体顶层共享或独立，视实现而定）
        // 假设SymTable能正确处理当前作用域查找
        if (existing && existing->scopeLevel == symTable.getScopeDepth()) {
             errors.push_back("Redefinition of parameter " + node.entry->getName() + " at line " + std::to_string(node.line_num));
             return false;
        }

        // 2. 构建属性
        VarAttr attr;
        attr.type = node.type; // Base type
        attr.scopeLevel = symTable.getScopeDepth();
        attr.isConstDecl = false; // Parameters are not const

        // 3. 处理数组维度
        if (node.dims && !node.dims->empty()) {
            // 数组参数退化为指针
            // int a[][3] -> int (*a)[3]
            // 这里我们简单处理：标记为指针类型，并记录维度
            attr.type = TypeFactory::getPtrType(node.type);
            
            for (size_t i = 0; i < node.dims->size(); ++i) {
                ExprNode* dimExpr = (*node.dims)[i];
                if (dimExpr) {
                    apply(*this, *dimExpr);
                    // 第一维可以是空的 (nullptr or implicit)，但在AST中可能是空指针?
                    // 实际上 parser 处理 int a[] 时，第一个维度可能是空的
                    // 如果 dimExpr 非空，必须是常量
                    if (dimExpr->attr.val.isConstexpr) {
                        attr.arrayDims.push_back(dimExpr->attr.val.getInt());
                    } else {
                         // 第一维如果是空，parser可能没放进dims，或者是nullptr?
                         // 假设只有非空维度才在dims里，除了第一个可能为空
                         // 但 SysY 语法 int a[][3]
                         errors.push_back("Array dimension must be constant at line " + std::to_string(node.line_num));
                    }
                } else {
                    // Empty dimension (e.g. first one)
                    attr.arrayDims.push_back(0); // 0 represents omitted dimension
                }
            }
        }

        // 4. 加入符号表
        symTable.addSymbol(node.entry, attr);
        return true;
    }

    bool ASTChecker::visit(VarDeclaration& node)
    {
        // TODO(Lab3-1): 实现变量声明的语义检查
        // 遍历声明列表，检查重定义，处理数组维度和初始化，将符号加入符号表
        
        if (!node.decls) return true;

        for (auto* decl : *node.decls) {
            if (!decl) continue;

            LeftValExpr* lval = dynamic_cast<LeftValExpr*>(decl->lval);
            if (!lval) continue; // Should not happen

            Entry* entry = lval->entry;

            // 1. 检查重定义
            VarAttr* existing = symTable.getSymbol(entry);
            if (existing && existing->scopeLevel == symTable.getScopeDepth()) {
                errors.push_back("Redefinition of variable " + entry->getName() + " at line " + std::to_string(node.line_num));
                continue; 
            }

            // 2. 准备属性
            VarAttr attr;
            attr.type = node.type;
            attr.isConstDecl = node.isConstDecl;
            attr.scopeLevel = symTable.getScopeDepth();

            // 3. 处理数组维度
            if (lval->indices && !lval->indices->empty()) {
                for (auto* dimExpr : *lval->indices) {
                    apply(*this, *dimExpr);
                    if (!dimExpr->attr.val.isConstexpr) {
                        errors.push_back("Array dimension must be constant at line " + std::to_string(node.line_num));
                    }
                    int dim = dimExpr->attr.val.getInt();
                    if (dim <= 0) {
                         errors.push_back("Array dimension must be positive at line " + std::to_string(node.line_num));
                    }
                    attr.arrayDims.push_back(dim);
                }
            }

            // 4. 处理初始化
            if (decl->init) {
                apply(*this, *decl->init);
                // TODO: Check initializer type match against attr.type and attr.arrayDims
                // For now, we assume logic inside Initializer visit handles expression checks
                // We should update attr.initList if needed
                
                // 如果是 const 变量，初始化值必须是常量
                if (attr.isConstDecl) {
                    // Check if init val is constant
                    // Initializer -> init_val (ExprNode)
                    // InitializerList -> list
                    // 简单起见，只检查顶层
                    Initializer* simpleInit = dynamic_cast<Initializer*>(decl->init);
                    if (simpleInit && simpleInit->init_val) {
                        if (!simpleInit->init_val->attr.val.isConstexpr) {
                             errors.push_back("Const variable must be initialized with constant at line " + std::to_string(node.line_num));
                        }
                        // 保存常量值到 initList (simplified)
                        attr.initList.push_back(simpleInit->init_val->attr.val.value);
                    }
                }
            }

            // 5. 加入符号表
            symTable.addSymbol(entry, attr);
            
            // Register global symbol if at depth 0
            if (symTable.getScopeDepth() == 0) {
                glbSymbols[entry] = attr;
            }
        }
        return true;
    }
}  // namespace FE::AST
