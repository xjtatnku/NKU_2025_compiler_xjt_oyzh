#include <frontend/symbol/symbol_table.h>
#include <debug.h>

namespace FE::Sym
{
    void SymTable::reset_impl() { TODO("Lab3-1: Reset symbol table"); }

    void SymTable::enterScope_impl() { TODO("Lab3-1: Enter new scope"); }

    void SymTable::exitScope_impl() { TODO("Lab3-1: Exit current scope"); }

    void SymTable::addSymbol_impl(Entry* entry, FE::AST::VarAttr& attr)
    {
        (void)entry;
        (void)attr;
        TODO("Lab3-1: Add symbol to current scope");
    }

    FE::AST::VarAttr* SymTable::getSymbol_impl(Entry* entry)
    {
        (void)entry;
        TODO("Lab3-1: Get symbol from symbol table");
    }

    bool SymTable::isGlobalScope_impl() { TODO("Lab3-1: Check if current scope is global scope"); }

    int SymTable::getScopeDepth_impl() { TODO("Lab3-1: Get current scope depth"); }
}  // namespace FE::Sym
