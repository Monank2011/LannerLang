#pragma once

#include "../parser/ast.hpp"
#include "../sema/comptime_value.hpp"
#include <map>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class LLVMCodeGenerator {
public:
    std::string generate(const Program& program, bool includeRuntime = true, const std::string& targetTriple = {});

private:
    struct LocalBinding {
        TypeNode* type = nullptr;
        std::string addr;
        bool moved = false;
        bool cleaned = false;
    };

    struct LoopContext {
        std::string conditionLabel;
        std::string continueLabel;
        std::string exitLabel;
        std::size_t cleanupDepth = 0;
    };

    const FunctionDecl* currentFunction = nullptr;
    std::vector<std::map<std::string, LocalBinding>> localScopes;
    std::vector<std::vector<std::string>> scopeOrder;
    std::map<std::string, const FunctionDecl*> functions;
    std::map<std::string, const StructDecl*> structs;
    std::map<std::string, const EnumDecl*> enums;
    std::map<std::string, const StaticDecl*> statics;
    std::map<std::string, ComptimeValue> comptimeGlobals;
    std::vector<std::map<std::string, ComptimeValue>> comptimeScopes;
    std::string comptimeLiteral(const ComptimeValue& value);
    std::vector<LoopContext> loopLabels;
    std::vector<std::string> body;
    std::vector<std::unique_ptr<TypeNode>> ownedLocalTypes;
    int tempCounter = 0;
    int labelCounter = 0;
    bool blockTerminated = false;
    std::vector<std::string> stringLiterals;
    bool includeRuntime = true;
    std::string targetTriple;
    int pointerBits = 64;

    std::unique_ptr<TypeNode> cloneType(const TypeNode* type) const;
    std::string llvmType(const TypeNode* type) const;
    std::string exprLLVMType(const Expr* expr) const;
    std::string emitExpr(const Expr* expr);
    std::string emitImplicitOptionalWrap(const Expr* expr);
    void emitStmt(const Stmt* stmt);
    bool emitBlock(const std::vector<std::unique_ptr<Stmt>>& stmts);
    void emitFunction(const FunctionDecl& fn, std::string& out);

    std::string emitLValueAddress(const Expr* expr);
    const StructField* findStructField(const TypeNode* type, const std::string& field) const;
    std::string normalizeIntLiteral(const std::string& literal) const;
    std::string llvmFloatLiteral(const Expr* expr) const;
    std::string normalizeIndexToI64(const Expr* expr, const std::string& value);
    void emitBoundsCheck(const Expr* index, const std::string& indexI64, std::uint64_t size);
    void emitDynamicBoundsCheck(const std::string& indexI64, const std::string& lengthI64);

    std::string emitArenaCreate(const Expr* expr);
    std::string emitDynamicArrayPush(const Expr* target, const Expr* value, int line);
    std::string dynamicArrayElementType(const TypeNode* arrayType) const;
    std::string emitTypeSize(const std::string& elementType);
    std::string emitTypeAlign(const std::string& elementType);
    std::string arenaAddressFor(const std::string& arenaName, int line);
    void cleanupBinding(const std::string& name, LocalBinding& local);
    void cleanupCurrentScope();
    void cleanupScopesFrom(std::size_t depth);
    void cleanupAllScopes();
    void markMovedArgument(const Expr* arg, const TypeNode* paramType);
    std::string emitDynamicArrayLiteral(const Expr* expr, const std::string& arenaAddress);
    std::string emitBuiltinCall(const Expr* expr);
    std::string emitStaticConstant(const Expr* expr, const TypeNode* type);
    void emitStaticDecl(const StaticDecl& decl, std::string& out);
    void emitThreadWrapper(const FunctionDecl& fn, std::string& out);
    std::string emitPrintCall(const Expr* expr);
    std::string llvmStringLiteral(const std::string& value);
    bool isWebTarget() const;

    std::string newTemp(const std::string& prefix = "t");
    std::string newLabel(const std::string& prefix = "label");
    [[noreturn]] void unsupported(const std::string& message, int line) const;

    const LocalBinding* lookupLocal(const std::string& name) const;
    LocalBinding* lookupLocal(const std::string& name);
    void pushScope();
    void popScope();
    const ComptimeValue* lookupComptime(const std::string& name) const;

    static bool isIntegerLLVM(const std::string& type);
    static bool isUnsignedType(const TypeNode* type);
    static bool isSignedType(const TypeNode* type);
};
