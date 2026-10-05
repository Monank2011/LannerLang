#pragma once

#include "symbol_table.hpp"
#include "../parser/ast.hpp"
#include <map>
#include <memory>
#include <set>
#include <cstddef>
#include <string>
#include <vector>

class TypeChecker {
public:
    explicit TypeChecker(SymbolTable& symtab) : symbols(symtab) {}

    void setTargetTriple(const std::string& triple);
    void checkProgram(Program& program);
    void checkFunction(FunctionDecl& fn);
    void checkStmt(const Stmt* stmt);
    std::unique_ptr<TypeNode> inferExprType(const Expr* expr);

    static std::string typeToString(const TypeNode* type);
    static bool typesEqual(const TypeNode* a, const TypeNode* b);
    static bool isCopyType(const TypeNode* type);

private:
    SymbolTable& symbols;
    std::vector<std::unique_ptr<TypeNode>> typePool;
    std::map<std::string, FunctionDecl*> functions;
    std::map<std::string, StructDecl*> structs;
    std::map<std::string, EnumDecl*> enums;
    std::map<std::string, StaticDecl*> statics;
    // When a function returns a reference/view whose lifetime is tied to one of
    // its reference/view parameters, record that parameter index so the caller
    // can reattach the borrow provenance to the caller's owner.
    std::map<std::string, std::size_t> functionReturnBorrowParam;
    std::string currentFunctionName;
    std::unique_ptr<TypeNode> currentReturnType;
    int loopDepth = 0;
    int unsafeDepth = 0;
    int pointerBits = 64;
    std::string targetTriple;
    std::size_t currentStmtSerial = 0;
    std::map<std::string, std::size_t> lastUseByName;

    static const std::set<std::string> primitiveTypes;

    void registerTopLevel(Program& program);
    void validateType(TypeNode* type, int line);
    void checkFunctionSignature(const FunctionDecl& fn);
    bool isExternAbiType(const TypeNode* type) const;
    void checkStructDecl(const StructDecl& st);
    void checkEnumDecl(const EnumDecl& en) const;
    void checkStaticDecl(StaticDecl& st);
    bool checkBlock(const std::vector<std::unique_ptr<Stmt>>& body);
    bool alwaysTerminates(const Stmt* stmt) const;

    std::unique_ptr<TypeNode> checkExpr(const Expr* expr, const TypeNode* expected);
    std::unique_ptr<TypeNode> checkBinaryOp(const Expr* expr, const TypeNode* expected = nullptr);
    std::unique_ptr<TypeNode> checkCall(const Expr* expr, const TypeNode* expected);
    std::unique_ptr<TypeNode> checkFieldAccess(const Expr* expr);
    std::unique_ptr<TypeNode> checkStructLiteral(const Expr* expr);
    std::unique_ptr<TypeNode> checkIsMatch(const Expr* expr);
    std::unique_ptr<TypeNode> checkResultLiteral(const Expr* expr, const TypeNode* expected);
    std::unique_ptr<TypeNode> checkCast(const Expr* expr);
    std::unique_ptr<TypeNode> checkLValue(const Expr* expr);

    bool isNumeric(const TypeNode* type) const;
    bool isInteger(const TypeNode* type) const;
    bool isBoolean(const TypeNode* type) const;
    bool isVoid(const TypeNode* type) const;
    bool isResult(const TypeNode* type) const;
    bool isAtomic(const TypeNode* type) const;
    static bool isVector(const TypeNode* type);
    static bool isStaticConstantExpr(const Expr* expr);
    bool isView(const TypeNode* type) const;
    bool isExclusiveView(const TypeNode* type) const;
    bool isWebTarget() const;
    bool isAndroidTarget() const;
    bool isIOSTarget() const;
    bool isMobileTarget() const;
    static stable::memory::StorageOrigin inferOrigin(const Symbol* owner, const std::string& ownerName);
    std::optional<stable::memory::BorrowRecord> registerBorrow(Symbol* owner, const TypeNode* borrowedType, const std::string& ownerName) const;
    void releaseBorrow(Symbol* borrower);
    void releaseBorrowsAtLastUse();
    void collectExprUses(const Expr* expr, std::size_t serial);
    void collectStmtUses(const Stmt* stmt, std::size_t& serial);
    void collectBlockUses(const std::vector<std::unique_ptr<Stmt>>& body, std::size_t& serial);
    bool requiresOwnershipTransfer(const TypeNode* type) const;
    void rejectBorrowedOwnerProjection(const Expr* expr, const TypeNode* type, const char* context) const;
    void rejectMoveWhileBorrowed(const Symbol* source, const Expr* expr, const char* context) const;
    void recordReturnedBorrow(const std::string& functionName, const std::string& ownerName, const Expr* expr);

    std::unique_ptr<TypeNode> makeType(const std::string& name, bool array = false) const;
    std::unique_ptr<TypeNode> cloneType(const TypeNode* type) const;
    std::unique_ptr<TypeNode> makeResultType(const TypeNode* ok, const TypeNode* err) const;
    bool compatible(const TypeNode* actual, const TypeNode* expected) const;
    bool literalFits(const Expr* expr, const TypeNode* type) const;
    void popCheckedScope();
    [[noreturn]] void error(const Expr* expr, const std::string& message) const;
};
