#pragma once
#include "../parser/ast.hpp"
#include "comptime_value.hpp"
#include "../memory/memory_model.hpp"
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct Symbol {
    std::uint64_t bindingId = 0;
    std::string name;
    TypeNode* type = nullptr;
    lanner::memory::BindingState memory;
    bool isConst = false;
    bool isComptime = false;
    bool isParameter = false;
    bool isStatic = false;
    std::optional<ComptimeValue> comptimeValue;
    int declaredLine = 0;
};

class Scope {
public:
    explicit Scope(Scope* parent = nullptr) : parentScope(parent) {}
    void declare(const std::string& name, Symbol sym);
    Symbol* resolve(const std::string& name);
    Symbol* resolveLocal(const std::string& name);
    const std::map<std::string, Symbol>& entries() const { return symbols; }
    Scope* parent() const { return parentScope; }

private:
    std::map<std::string, Symbol> symbols;
    Scope* parentScope;
};

class SymbolTable {
public:
    SymbolTable();
    void pushScope();
    void popScope();
    void declare(const std::string& name, Symbol sym);
    Symbol* resolve(const std::string& name);
    Symbol* resolveBinding(std::uint64_t bindingId);
    Scope* currentScope() { return scopes.back().get(); }
    const Scope* currentScope() const { return scopes.back().get(); }
    size_t depth() const { return scopes.size(); }

private:
    std::vector<std::unique_ptr<Scope>> scopes;
    std::uint64_t nextBindingId = 1;
};
