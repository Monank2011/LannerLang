#pragma once

#include "hir.hpp"
#include "../sema/comptime_value.hpp"
#include <map>
#include <set>
#include <string>
#include <vector>

namespace stable::hir {

class Lowerer {
public:
    Module lowerProgram(const Program& program);

private:
    const Program* program = nullptr;
    std::map<std::string, const FunctionDecl*> functions;
    std::map<std::string, EnumDecl*> enums;
    std::map<std::string, StructDecl*> structs;
    std::map<std::string, ComptimeValue> comptimeGlobals;
    std::vector<std::map<std::string, ComptimeValue>> comptimeScopes;
    Function current;
    std::size_t currentBlock = 0;
    int tempCounter = 0;
    int blockCounter = 0;
    std::vector<std::map<std::string, std::string>> localScopes;
    std::vector<std::vector<std::string>> scopeOrder;
    std::vector<std::string> breakTargets;
    std::vector<std::string> continueTargets;
    std::vector<std::size_t> loopScopeDepths;
    std::set<std::string> initializedSlots;
    std::set<std::string> movedSlots;
    std::set<std::string> destroyedSlots;

    void registerProgram(const Program& p);
    Function lowerFunction(const FunctionDecl& fn);
    void lowerStmt(const Stmt* stmt);
    void lowerBlock(const std::vector<std::unique_ptr<Stmt>>& stmts);
    std::string lowerExpr(const Expr* expr);
    std::string lowerImplicitOptionalWrap(const Expr* expr);
    std::string lowerCall(const Expr* expr);
    std::string lowerFieldEnum(const Expr* expr);
    Type lowerType(const TypeNode* type) const;
    std::string lowerStructFieldValue(const Expr* expr);
    std::size_t structFieldIndex(const TypeNode* type, const std::string& field) const;
    std::string addressOfLocal(const std::string& sourceName, int line);
    std::string lowerAggregateIndexAddress(const Expr* expr);
    std::string lowerLValueAddress(const Expr* expr);
    void emitStoreLocal(const std::string& slot, const Type& type, const std::string& value, const Expr* rhs);
    void markMovedIfOwned(const Expr* expr);
    void cleanupCurrentScope();
    void cleanupScopesFrom(std::size_t firstDepth);
    void cleanupScopes();
    void cleanupLocal(const std::string& slot, const Type& type);
    void emitDestroyAt(const std::string& address, const Type& type);

    std::string newTemp();
    std::string newBlockLabel(const std::string& prefix);
    BasicBlock& block();
    bool terminated() const;
    void emit(const Instruction& inst);
    void ensureBranchTo(const std::string& label);

    void pushScope();
    void popScope();
    const ComptimeValue* lookupComptime(const std::string& name) const;
    std::string declareLocal(const std::string& sourceName);
    std::string lookupLocal(const std::string& sourceName) const;
    std::string lookupCurrentLocal(const std::string& sourceName) const;

    Type exprType(const Expr* expr) const;
    Type declaredType(const TypeNode* type) const;
    [[noreturn]] void unsupported(const std::string& message, int line) const;
};

} // namespace stable::hir
