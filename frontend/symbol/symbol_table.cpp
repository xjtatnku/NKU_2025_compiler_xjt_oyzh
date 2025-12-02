#include <frontend/symbol/symbol_table.h>
#include <debug.h>

namespace FE::Sym
{
    void SymTable::reset_impl()
    {
        scopes.clear();
        scopes.emplace_back();
    }

    void SymTable::enterScope_impl() { scopes.emplace_back(); }

    void SymTable::exitScope_impl()
    {
        if (scopes.size() > 1) scopes.pop_back();
    }

    void SymTable::addSymbol_impl(Entry* entry, FE::AST::VarAttr& attr)
    {
        if (scopes.empty()) scopes.emplace_back();
        scopes.back()[entry] = attr;
    }

    FE::AST::VarAttr* SymTable::getSymbol_impl(Entry* entry)
    {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
        {
            auto search = it->find(entry);
            if (search != it->end())
            {
                return &search->second;
            }
        }
        return nullptr;
    }

    bool SymTable::isGlobalScope_impl() { return scopes.size() <= 1; }

    int SymTable::getScopeDepth_impl() { return static_cast<int>(scopes.size()) - 1; }
}  // namespace FE::Sym
