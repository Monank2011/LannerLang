#include "symbol_table.hpp"
#include <stdexcept>

void Scope::declare(const std::string& name, Symbol sym) {
    if (symbols.find(name) != symbols.end()) {
        throw std::runtime_error("Redeclaration of '" + name + "' in the same scope (line " +
                                 std::to_string(sym.declaredLine) + ")");
    }
    symbols.emplace(name, std::move(sym));
}

Symbol* Scope::resolveLocal(const std::string& name) {
    auto it = symbols.find(name);
    return it == symbols.end() ? nullptr : &it->second;
}

Symbol* Scope::resolve(const std::string& name) {
    if (auto* local = resolveLocal(name)) return local;
    return parentScope ? parentScope->resolve(name) : nullptr;
}

SymbolTable::SymbolTable() {
    scopes.push_back(std::make_unique<Scope>(nullptr));
}

void SymbolTable::pushScope() {
    scopes.push_back(std::make_unique<Scope>(currentScope()));
}

void SymbolTable::popScope() {
    if (scopes.size() <= 1) throw std::runtime_error("Cannot pop the global scope");
    scopes.pop_back();
}

void SymbolTable::declare(const std::string& name, Symbol sym) {
    if (sym.bindingId == 0) sym.bindingId = nextBindingId++;
    currentScope()->declare(name, std::move(sym));
}

Symbol* SymbolTable::resolve(const std::string& name) {
    return currentScope()->resolve(name);
}

Symbol* SymbolTable::resolveBinding(std::uint64_t bindingId) {
    if (bindingId == 0) return nullptr;
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        for (auto& entry : (*it)->entries()) {
            if (entry.second.bindingId == bindingId) return const_cast<Symbol*>(&entry.second);
        }
    }
    return nullptr;
}
