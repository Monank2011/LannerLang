#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "../memory/memory_model.hpp"

struct Expr;

struct TypeNode {
    ~TypeNode();
    std::string name;
    bool isArray = false;                 // []T, a runtime-sized owned sequence
    int line = 0;
    int column = 1;
    std::optional<std::uint64_t> fixedArraySize; // [N]T, inline fixed-size array
    // Parsed only when N is a comptime expression; semantic analysis resolves it
    // into fixedArraySize before code generation.
    std::unique_ptr<Expr> fixedArraySizeExpr;
    bool isOptional = false;
    bool isReference = false;             // source-level &T / &mut T mode
    bool isMutable = false;               // true for exclusive &mut T
    bool isRawPointer = false;             // unsafe raw pointer *T / *mut T
    bool isUnsafeFunction = false;         // unsafe fn(...) function-pointer type
    // Function pointer type: name == "fn", generics = parameter types followed by return type.
    std::vector<std::unique_ptr<TypeNode>> generics;
    // Canonical storage provenance for ownership and borrow escape analysis.
    // `origin` describes the value's own backing storage; `nestedOrigins`
    // conservatively records short-lived storage/borrow origins captured inside
    // aggregate payloads. It is semantic metadata and is ignored for nominal type
    // equality.
    lanner::memory::StorageOrigin origin;
    std::vector<lanner::memory::StorageOrigin> nestedOrigins;
};

enum class ExprKind {
    IntLit, FloatLit, StringLit, BoolLit, Identifier,
    BinaryOp, UnaryOp, Call, FieldAccess,
    StructLit, ArrayLit,
    ArenaAlloc,
    Reference,
    NoneLit,
    OkLit, ErrLit,
    IsMatch,
    Cast,
    Index,
    Slice
};

struct Expr {
    ~Expr();
    ExprKind kind;
    int line = 0;
    int column = 1;
    std::string strValue;
    std::string op;
    std::unique_ptr<TypeNode> castType;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> args;
    std::unique_ptr<Expr> target;
    std::string field;
    std::string elementTypeName;
    std::string structName;
    std::vector<std::pair<std::string, std::unique_ptr<Expr>>> fields;
    std::unique_ptr<Expr> value;
    std::unique_ptr<Expr> arena;
    std::string matchKind;
    std::string bindingName;
    bool implicitOptionalWrap = false; // semantic annotation: payload coerced into T?

    // Semantic annotation. The checker owns the cloned type and the AST keeps it
    // so later IR/codegen phases do not have to re-infer expression types.
    mutable std::unique_ptr<TypeNode> checkedType;
};

inline TypeNode::~TypeNode() = default;
inline Expr::~Expr() = default;

enum class StmtKind {
    ExprStmt, Assign, ConstAssign, Return, Continue, Break,
    If, While, For, Guard,
    ComptimeDecl, UnsafeBlock
};

struct Stmt {
    StmtKind kind;
    int line = 0;
    int column = 1;
    std::unique_ptr<Expr> expr;
    std::string assignTarget;
    std::unique_ptr<TypeNode> declaredType;
    std::unique_ptr<Expr> assignValue;
    std::string loopVar;
    std::unique_ptr<Expr> iterable;      // collection, or range start for `for i in a..b`
    std::unique_ptr<Expr> rangeEnd;      // Lanner 3.0.0: non-null for `a..b` / `a..=b` loops
    bool rangeInclusive = false;         // true for `a..=b`
    std::vector<std::unique_ptr<Stmt>> body;
    std::vector<std::unique_ptr<Stmt>> elseBody;
    std::unique_ptr<Expr> guardCondition;
    std::unique_ptr<Stmt> guardBody;
    std::string comptimeName;
    std::unique_ptr<Expr> comptimeValue;
};

struct Param { std::string name; std::unique_ptr<TypeNode> type; };
struct FunctionDecl {
    std::string name;
    bool isExtern = false;
    bool isUnsafe = false;
    std::vector<Param> params;
    std::unique_ptr<TypeNode> returnType;
    std::vector<std::unique_ptr<Stmt>> body;
    int line = 0;
    int column = 1;
};
struct StructField { std::string name; std::unique_ptr<TypeNode> type; };
struct StructDecl {
    std::string name;
    std::vector<StructField> fields;
    bool isPacked = false;
    int line = 0;
    int column = 1;
};

struct EnumVariant {
    std::string name;
    std::int64_t value = 0;
};
struct EnumDecl {
    std::string name;
    std::vector<EnumVariant> variants;
    int line = 0;
    int column = 1;
};

enum class DeclKind { Function, Struct, Enum, ComptimeGlobal, Static };

struct StaticDecl {
    std::string name;
    std::unique_ptr<TypeNode> type;
    std::unique_ptr<Expr> value;
    bool isMutable = false;
    std::uint32_t alignment = 0;
    std::string section;
    bool isThreadLocal = false;
    bool isExtern = false;
    int line = 0;
    int column = 1;
};

struct TopLevelDecl {
    DeclKind kind;
    std::unique_ptr<FunctionDecl> fn;
    std::unique_ptr<StructDecl> st;
    std::unique_ptr<EnumDecl> en;
    std::unique_ptr<Stmt> comptimeGlobal;
    std::unique_ptr<StaticDecl> staticDecl;
};
struct Program { std::vector<std::unique_ptr<TopLevelDecl>> decls; };
