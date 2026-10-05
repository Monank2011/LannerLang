#include "typechecker.hpp"
#include "../comptime/comptime_eval.hpp"
#include <algorithm>
#include <functional>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <functional>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <tuple>

const std::set<std::string> TypeChecker::primitiveTypes = {
    "void", "bool", "string",
    "i8", "i16", "i32", "i64", "i128", "isize",
    "u8", "u16", "u32", "u64", "u128", "usize",
    "f32", "f64", "Arena", "View", "EditView", "Atomic", "Thread", "Socket", "Poller", "Mutex", "RwLock", "Condvar", "Semaphore", "Process", "Buffer", "Tensor", "GradTape", "GameWindow", "GameRenderer", "GameTexture", "GameAudio", "Regex", "v128", "v256", "v512"
};

namespace {

bool isIntName(const std::string& n) {
    if (n == "isize" || n == "usize") return true;
    return n.size() > 1 && (n[0] == 'i' || n[0] == 'u') &&
           std::all_of(n.begin() + 1, n.end(), [](char c) {
               return std::isdigit(static_cast<unsigned char>(c)) != 0;
           });
}

bool isFloatName(const std::string& n) { return n == "f32" || n == "f64"; }

void rememberOrigin(std::vector<stable::memory::StorageOrigin>& origins,
                    const stable::memory::StorageOrigin& origin) {
    if (!origin.valid()) return;
    for (const auto& existing : origins) {
        if (existing.kind == origin.kind && existing.binding == origin.binding) return;
    }
    origins.push_back(origin);
}

bool capturesBackingLifetime(const TypeNode* type) {
    if (!type) return false;
    // Local/parameter origins on ordinary owning values identify the current
    // owner binding, not storage that is being borrowed by another value. They
    // must not make an otherwise movable struct or scalar non-returnable.
    if ((type->isReference || type->name == "View" || type->name == "EditView") && type->origin.valid()) return true;
    if (type->isArray && type->origin.kind == stable::memory::StorageOriginKind::Arena) return true;
    return false;
}

void mergeNestedOrigins(TypeNode* destination, const TypeNode* source) {
    if (!destination || !source) return;
    if (capturesBackingLifetime(source)) rememberOrigin(destination->nestedOrigins, source->origin);
    for (const auto& origin : source->nestedOrigins) rememberOrigin(destination->nestedOrigins, origin);
}

bool hasShortLivedNestedOrigin(const TypeNode* type) {
    if (!type) return false;
    if (capturesBackingLifetime(type) && type->origin.isShortLived()) return true;
    for (const auto& origin : type->nestedOrigins) {
        if (origin.isShortLived()) return true;
    }
    for (const auto& g : type->generics) {
        if (hasShortLivedNestedOrigin(g.get())) return true;
    }
    return false;
}

const stable::memory::StorageOrigin* firstShortLivedOrigin(const TypeNode* type) {
    if (!type) return nullptr;
    if (capturesBackingLifetime(type) && type->origin.isShortLived()) return &type->origin;
    for (const auto& origin : type->nestedOrigins) {
        if (origin.isShortLived()) return &origin;
    }
    for (const auto& g : type->generics) {
        if (const auto* found = firstShortLivedOrigin(g.get())) return found;
    }
    return nullptr;
}

std::string shortLivedOriginDescription(const TypeNode* type) {
    const auto* origin = firstShortLivedOrigin(type);
    if (!origin) return "short-lived storage";
    if (origin->kind == stable::memory::StorageOriginKind::Arena) {
        return "arena '" + origin->binding + "'";
    }
    if (origin->kind == stable::memory::StorageOriginKind::Local) {
        return "local storage '" + origin->binding + "'";
    }
    return stable::memory::originName(origin->kind);
}

int integerBits(const std::string& n) {
    if (n == "isize" || n == "usize") return 64;
    if (!isIntName(n)) return 0;
    try { return std::stoi(n.substr(1)); }
    catch (...) { return 0; }
}

bool isSignedInteger(const std::string& n) {
    return isIntName(n) && n != "usize" && n[0] == 'i';
}

std::uint64_t parseUnsignedLiteralText(const std::string& s) {
    int base = 10;
    std::size_t start = 0;
    if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        base = 16;
        start = 2;
    } else if (s.size() > 2 && s[0] == '0' && (s[1] == 'b' || s[1] == 'B')) {
        base = 2;
        start = 2;
    }
    std::uint64_t value = 0;
    const auto result = std::from_chars(s.data() + start, s.data() + s.size(), value, base);
    if (result.ec != std::errc{} || result.ptr != s.data() + s.size()) {
        throw std::runtime_error("invalid integer literal '" + s + "'");
    }
    return value;
}

} // namespace

void TypeChecker::setTargetTriple(const std::string& triple) {
    targetTriple = triple;
    pointerBits = 64;
    const auto has = [&](const char* part) { return triple.find(part) != std::string::npos; };
    if (has("wasm32") || has("i386") || has("i486") || has("i586") || has("i686") ||
        has("armv7") || has("armv6") || has("thumbv7") || has("thumbv6") ||
        has("riscv32") || has("mipsel") || has("mips-") || has("powerpc-")) pointerBits = 32;
}

bool TypeChecker::isWebTarget() const { return targetTriple.rfind("wasm32", 0) == 0; }

bool TypeChecker::isAndroidTarget() const { return targetTriple.find("android") != std::string::npos; }
bool TypeChecker::isIOSTarget() const { return targetTriple.find("apple-ios") != std::string::npos; }
bool TypeChecker::isMobileTarget() const { return isAndroidTarget() || isIOSTarget(); }

std::unique_ptr<TypeNode> TypeChecker::makeType(const std::string& name, bool array) const {
    auto t = std::make_unique<TypeNode>();
    t->name = name;
    t->isArray = array;
    return t;
}

std::unique_ptr<TypeNode> TypeChecker::cloneType(const TypeNode* type) const {
    if (!type) return nullptr;
    auto c = std::make_unique<TypeNode>();
    c->name = type->name;
    c->line = type->line;
    c->column = type->column;
    c->isArray = type->isArray;
    c->fixedArraySize = type->fixedArraySize;
    c->isOptional = type->isOptional;
    c->isReference = type->isReference;
    c->isMutable = type->isMutable;
    c->isRawPointer = type->isRawPointer;
    c->isUnsafeFunction = type->isUnsafeFunction;
    c->origin = type->origin;
    c->nestedOrigins = type->nestedOrigins;
    for (const auto& g : type->generics) c->generics.push_back(cloneType(g.get()));
    return c;
}

std::unique_ptr<TypeNode> TypeChecker::makeResultType(const TypeNode* ok, const TypeNode* err) const {
    auto r = makeType("Result");
    r->generics.push_back(cloneType(ok));
    r->generics.push_back(cloneType(err));
    return r;
}

std::string TypeChecker::typeToString(const TypeNode* type) {
    if (!type) return "<null>";
    std::string s;
    if (type->isReference) s += std::string("&") + (type->isMutable ? "mut " : "");
    else if (type->isRawPointer) s += std::string("*") + (type->isMutable ? "mut " : "");
    if (type->isArray) {
        if (type->fixedArraySize) s += "[" + std::to_string(*type->fixedArraySize) + "]";
        else s += "[]";
    }
    s += type->name;
    if (!type->generics.empty()) {
        s += "[";
        for (std::size_t i = 0; i < type->generics.size(); ++i) {
            if (i) s += ", ";
            s += typeToString(type->generics[i].get());
        }
        s += "]";
    }
    if (type->isOptional) s += "?";
    if (type->origin.kind == stable::memory::StorageOriginKind::Arena) s += " in " + type->origin.binding;
    return s;
}

bool TypeChecker::typesEqual(const TypeNode* a, const TypeNode* b) {
    if (!a || !b) return a == b;
    if (a->name != b->name || a->isArray != b->isArray ||
        a->fixedArraySize != b->fixedArraySize ||
        a->isOptional != b->isOptional || a->isReference != b->isReference ||
        a->isRawPointer != b->isRawPointer || a->isMutable != b->isMutable ||
        a->isUnsafeFunction != b->isUnsafeFunction ||
        a->generics.size() != b->generics.size()) return false;
    for (std::size_t i = 0; i < a->generics.size(); ++i) {
        if (!typesEqual(a->generics[i].get(), b->generics[i].get())) return false;
    }
    return true;
}

bool TypeChecker::isCopyType(const TypeNode* type) {
    if (!type) return false;
    if (type->isReference) return !type->isMutable;
    if (type->isRawPointer) return true;
    if (type->isOptional) {
        // Optionality does not make an owning type copyable. A primitive/View
        // optional remains copyable, while arrays, arenas, EditViews and other
        // aggregate owners retain their move-only semantics.
        if (type->name == "View") return true;
        if (type->name == "EditView") return false;
        if (type->isArray || type->fixedArraySize || !type->generics.empty()) return false;
        return primitiveTypes.count(type->name) != 0 && type->name != "Arena";
    }
    if (type->name == "View") return true;
    if (type->name == "EditView") return false;
    if (type->name == "Atomic" || type->name == "Thread" || type->name == "fn" || isVector(type)) return true;
    if (type->name == "Tensor" || type->name == "GradTape") return false;
    if (type->isArray || type->fixedArraySize || !type->generics.empty()) return false;
    return primitiveTypes.count(type->name) != 0 && type->name != "Arena";
}

bool TypeChecker::isNumeric(const TypeNode* type) const {
    return type && !type->isArray && !type->isReference && !type->isOptional &&
           (isIntName(type->name) || isFloatName(type->name));
}

bool TypeChecker::isInteger(const TypeNode* type) const {
    return type && !type->isArray && !type->isReference && !type->isOptional && isIntName(type->name);
}

bool TypeChecker::isBoolean(const TypeNode* type) const {
    return type && !type->isArray && !type->isReference && !type->isOptional && type->name == "bool";
}

bool TypeChecker::isVoid(const TypeNode* type) const {
    return type && !type->isArray && !type->isReference && !type->isOptional && type->name == "void";
}

bool TypeChecker::isResult(const TypeNode* type) const {
    return type && !type->isArray && !type->isReference && type->name == "Result" && type->generics.size() == 2;
}

bool TypeChecker::isAtomic(const TypeNode* type) const {
    return type && !type->isArray && !type->isReference && !type->isOptional &&
           type->name == "Atomic" && type->generics.size() == 1 && isInteger(type->generics[0].get());
}

bool TypeChecker::isVector(const TypeNode* type) {
    return type && !type->isArray && !type->isReference && !type->isOptional &&
           (type->name == "v128" || type->name == "v256" || type->name == "v512");
}

bool TypeChecker::isView(const TypeNode* type) const {
    return type && !type->isArray && !type->isReference && !type->isOptional &&
           (type->name == "View" || type->name == "EditView") && type->generics.size() == 1;
}

bool TypeChecker::isExclusiveView(const TypeNode* type) const {
    return isView(type) && type->name == "EditView";
}

bool TypeChecker::requiresOwnershipTransfer(const TypeNode* type) const {
    if (!type || type->isReference || isView(type) || isCopyType(type)) return false;
    // Enum values are represented as nominal scalars in the current AST. They are
    // not heap/resource owners even though the conservative static copy classifier
    // cannot see the program's enum table.
    if (!type->isArray && !type->isOptional && type->generics.empty() && enums.count(type->name)) return false;
    return true;
}

void TypeChecker::rejectBorrowedOwnerProjection(const Expr* expr, const TypeNode* type, const char* context) const {
    if (!expr || !requiresOwnershipTransfer(type)) return;
    if (expr->kind == ExprKind::FieldAccess) {
        // Nominal constants such as `MoveError.NoLegalMoves` use the same AST shape
        // as field access, but they do not project storage and therefore do not move.
        if (expr->target && expr->target->kind == ExprKind::Identifier &&
            !symbols.resolve(expr->target->strValue) &&
            (structs.count(expr->target->strValue) || enums.count(expr->target->strValue))) return;
        error(expr, std::string(context) + " cannot implicitly move a non-copy field/index value; move from an owning local instead");
    }
    if (expr->kind == ExprKind::Index) {
        error(expr, std::string(context) + " cannot implicitly move a non-copy field/index value; move from an owning local instead");
    }
}

void TypeChecker::rejectMoveWhileBorrowed(const Symbol* source, const Expr* expr, const char* context) const {
    if (!source) return;
    if (!stable::memory::mayMove(source->memory)) {
        error(expr, std::string(context) + " cannot move borrowed value '" + source->name + "'");
    }
}

void TypeChecker::recordReturnedBorrow(const std::string& functionName, const std::string& ownerName, const Expr* expr) {
    if (functionName.empty() || ownerName.empty()) return;
    auto fit = functions.find(functionName);
    if (fit == functions.end()) return;
    const auto* fn = fit->second;
    for (std::size_t i = 0; i < fn->params.size(); ++i) {
        if (fn->params[i].name != ownerName) continue;
        if (!fn->params[i].type->isReference && !isView(fn->params[i].type.get())) {
            error(expr, "returned borrow is not tied to a reference/view parameter");
        }
        auto it = functionReturnBorrowParam.find(functionName);
        if (it == functionReturnBorrowParam.end()) {
            functionReturnBorrowParam.emplace(functionName, i);
        } else if (it->second != i) {
            error(expr, "function '" + functionName + "' returns borrows tied to different parameters");
        }
        return;
    }
    error(expr, "returned borrow is not tied to a parameter of function '" + functionName + "'");
}

bool TypeChecker::compatible(const TypeNode* actual, const TypeNode* expected) const {
    if (!actual || !expected) return false;

    if (actual->name == "none" && expected->isOptional) return true;

    // A value of T can be contextually lifted into T?. The AST records this
    // conversion so each backend materializes the tagged optional representation.
    if (expected->isOptional && !actual->isOptional) {
        auto payload = cloneType(expected);
        payload->isOptional = false;
        return compatible(actual, payload.get());
    }

    // Untyped integer/float literals receive contextual primitive numeric types.
    if (actual->name == "i32" && !actual->isArray && !actual->isReference && !actual->isOptional &&
        !expected->isArray && !expected->isReference && !expected->isOptional && isIntName(expected->name)) {
        return true;
    }
    if (actual->name == "f64" && !actual->isArray && !actual->isReference && !actual->isOptional &&
        !expected->isArray && !expected->isReference && !expected->isOptional &&
        (expected->name == "f32" || expected->name == "f64")) {
        return true;
    }

    // Mutable raw pointers/references can be passed where a read-only pointer/reference is expected.
    // This is the low-level equivalent of const qualification: it does not grant mutation to the callee.
    if (actual->isRawPointer && expected->isRawPointer && actual->isMutable && !expected->isMutable &&
        actual->name == expected->name && actual->isArray == expected->isArray &&
        actual->isOptional == expected->isOptional && actual->isReference == expected->isReference &&
        actual->generics.size() == expected->generics.size()) {
        return true;
    }
    if (actual->isReference && expected->isReference && actual->isMutable && !expected->isMutable &&
        actual->name == expected->name && actual->isArray == expected->isArray &&
        actual->isOptional == expected->isOptional && actual->generics.size() == expected->generics.size()) {
        return true;
    }

    // Storage provenance is a safety property, not part of nominal type identity.
    // Strip it for contextual type compatibility while retaining it for escape checks.
    if (actual->origin.valid() && !expected->origin.valid()) {
        auto plain = cloneType(actual);
        plain->origin = {};
        return typesEqual(plain.get(), expected);
    }
    return typesEqual(actual, expected);
}

void TypeChecker::validateType(TypeNode* type, int line) {
    if (!type || type->name.empty()) {
        throw std::runtime_error("Invalid type at line " + std::to_string(line));
    }
    if (type->isRawPointer && (type->isArray || type->isOptional || type->isReference)) {
        throw std::runtime_error("raw pointer type cannot also be an array, optional, or reference at line " + std::to_string(line));
    }
    if (type->isOptional && type->name == "void") {
        throw std::runtime_error("void cannot be optional at line " + std::to_string(line));
    }
    if (type->isArray && type->name == "void") {
        throw std::runtime_error("void cannot be an array element type at line " + std::to_string(line));
    }

    if (!type->fixedArraySize && type->fixedArraySizeExpr) {
        ComptimeEvaluator evaluator(symbols);
        auto value = evaluator.evaluate(type->fixedArraySizeExpr.get());
        if (!std::holds_alternative<std::int64_t>(value)) {
            throw std::runtime_error("fixed array size must be an integer comptime expression at line " + std::to_string(line));
        }
        const auto n = std::get<std::int64_t>(value);
        if (n <= 0) {
            throw std::runtime_error("fixed arrays must have a positive size at line " + std::to_string(line));
        }
        type->fixedArraySize = static_cast<std::uint64_t>(n);
        type->fixedArraySizeExpr.reset();
    }
    if (type->fixedArraySize && *type->fixedArraySize == 0) {
        throw std::runtime_error("fixed arrays must have a positive size at line " + std::to_string(line));
    }

    if (type->name == "fn") {
        if (type->generics.empty()) {
            throw std::runtime_error("function-pointer type requires a return type at line " + std::to_string(line));
        }
        for (const auto& g : type->generics) {
            if (g->name == "fn" || g->isArray || g->isOptional || g->isReference || g->isRawPointer) continue;
            // Ordinary function-pointer argument/return types are validated below.
        }
        for (const auto& g : type->generics) validateType(g.get(), line);
        return;
    }

    const bool knownNominal = primitiveTypes.count(type->name) || structs.count(type->name) || enums.count(type->name);
    if (type->name != "Result" && !knownNominal) {
        throw std::runtime_error("Unknown type '" + type->name + "' at line " + std::to_string(line));
    }
    if (type->name == "Result" && type->generics.size() != 2) {
        throw std::runtime_error("Result requires exactly two type arguments at line " + std::to_string(line));
    }
    if ((type->name == "View" || type->name == "EditView" || type->name == "Atomic") && type->generics.size() != 1) {
        throw std::runtime_error(type->name + " requires exactly one type argument at line " + std::to_string(line));
    }
    if (type->name == "Atomic" && !type->generics.empty() && !isInteger(type->generics[0].get())) {
        throw std::runtime_error("Atomic currently supports integer payloads only at line " + std::to_string(line));
    }
    if ((type->name == "v128" || type->name == "v256" || type->name == "v512") && !type->generics.empty()) {
        throw std::runtime_error(type->name + " does not accept generic arguments at line " + std::to_string(line));
    }
    if (type->name != "Result" && type->name != "View" && type->name != "EditView" && type->name != "Atomic" &&
        type->name != "v128" && type->name != "v256" && type->name != "v512" && !type->generics.empty()) {
        throw std::runtime_error("Type '" + type->name + "' does not accept generic arguments at line " + std::to_string(line));
    }
    for (const auto& g : type->generics) validateType(g.get(), line);
}

void TypeChecker::checkStructDecl(const StructDecl& st) {
    std::set<std::string> seen;
    for (const auto& f : st.fields) {
        if (!seen.insert(f.name).second) {
            throw std::runtime_error("Duplicate field '" + f.name + "' in struct '" + st.name +
                                     "' at line " + std::to_string(st.line));
        }
        validateType(f.type.get(), st.line);
    }
}

void TypeChecker::checkEnumDecl(const EnumDecl& en) const {
    std::set<std::string> seen;
    std::set<std::int64_t> values;
    for (const auto& v : en.variants) {
        if (!seen.insert(v.name).second) {
            throw std::runtime_error("Duplicate enum variant '" + en.name + "." + v.name +
                                     "' at line " + std::to_string(en.line));
        }
        if (!values.insert(v.value).second) {
            throw std::runtime_error("Duplicate enum value " + std::to_string(v.value) +
                                     " in enum '" + en.name + "' at line " + std::to_string(en.line));
        }
    }
}

bool TypeChecker::isExternAbiType(const TypeNode* type) const {
    if (!type) return false;
    if (type->isArray || type->isOptional || type->isReference) return false;
    if (type->isRawPointer) return true;
    if (type->name == "fn" || type->name == "string") return true;
    if (type->name == "void" || type->name == "bool" || type->name == "i8" || type->name == "i16" ||
        type->name == "i32" || type->name == "i64" || type->name == "i128" || type->name == "isize" ||
        type->name == "u8" || type->name == "u16" || type->name == "u32" || type->name == "u64" ||
        type->name == "u128" || type->name == "usize" || type->name == "f32" || type->name == "f64") return true;
    return false;
}

void TypeChecker::checkFunctionSignature(const FunctionDecl& fn) {
    validateType(fn.returnType.get(), fn.line);
    std::set<std::string> seen;
    for (const auto& p : fn.params) {
        if (!seen.insert(p.name).second) {
            throw std::runtime_error("Duplicate parameter '" + p.name + "' in function '" + fn.name +
                                     "' at line " + std::to_string(fn.line));
        }
        validateType(p.type.get(), fn.line);
        if (fn.isExtern && !isExternAbiType(p.type.get())) {
            throw std::runtime_error("extern function '" + fn.name + "' uses an unsupported by-value ABI type in parameter '" +
                                     p.name + "'; pass aggregates through raw pointers or an explicit C wrapper");
        }
    }
    if (fn.isExtern && !isExternAbiType(fn.returnType.get())) {
        throw std::runtime_error("extern function '" + fn.name + "' uses an unsupported by-value ABI return type; return through a raw pointer or an explicit C wrapper");
    }
}

bool TypeChecker::isStaticConstantExpr(const Expr* expr) {
    if (!expr) return false;
    switch (expr->kind) {
        case ExprKind::IntLit:
        case ExprKind::FloatLit:
        case ExprKind::StringLit:
        case ExprKind::BoolLit:
            return true;
        case ExprKind::Identifier:
            return false;
        case ExprKind::UnaryOp:
            return (expr->op == "+" || expr->op == "-" || expr->op == "~" || expr->op == "!") &&
                   isStaticConstantExpr(expr->value.get());
        case ExprKind::BinaryOp:
            return isStaticConstantExpr(expr->left.get()) && isStaticConstantExpr(expr->right.get());
        case ExprKind::ArrayLit:
            for (const auto& a : expr->args) if (!isStaticConstantExpr(a.get())) return false;
            return true;
        case ExprKind::StructLit:
            for (const auto& f : expr->fields) if (!isStaticConstantExpr(f.second.get())) return false;
            return true;
        case ExprKind::NoneLit:
            return expr->strValue == "null";
        case ExprKind::Cast:
            return isStaticConstantExpr(expr->value.get());
        default:
            return false;
    }
}

void TypeChecker::checkStaticDecl(StaticDecl& st) {
    validateType(st.type.get(), st.line);
    if (!st.isExtern) {
        const bool functionAddress = st.value && st.value->kind == ExprKind::Identifier && functions.count(st.value->strValue) && st.type->name == "fn";
        if (!st.value || (!isStaticConstantExpr(st.value.get()) && !functionAddress)) {
            throw std::runtime_error("static '" + st.name + "' must use a compile-time constant initializer at line " +
                                     std::to_string(st.line));
        }
        auto actual = checkExpr(st.value.get(), st.type.get());
        if (!compatible(actual.get(), st.type.get())) {
            throw std::runtime_error("static '" + st.name + "' initializer has type " + typeToString(actual.get()) +
                                     ", expected " + typeToString(st.type.get()));
        }
    }
    Symbol sym;
    sym.name = st.name;
    sym.type = st.type.get();
    sym.isStatic = true;
    sym.isConst = !st.isMutable;
    sym.declaredLine = st.line;
    symbols.declare(st.name, sym);
}

void TypeChecker::registerTopLevel(Program& program) {
    functionReturnBorrowParam.clear();
    currentFunctionName.clear();
    functions.clear();
    structs.clear();
    enums.clear();
    statics.clear();

    for (auto& d : program.decls) {
        switch (d->kind) {
            case DeclKind::Function:
                if (!d->fn) throw std::runtime_error("Invalid function declaration");
                if (functions.count(d->fn->name) || structs.count(d->fn->name) || enums.count(d->fn->name) || statics.count(d->fn->name)) {
                    throw std::runtime_error("Duplicate top-level name '" + d->fn->name +
                                             "' at line " + std::to_string(d->fn->line));
                }
                functions[d->fn->name] = d->fn.get();
                break;
            case DeclKind::Struct:
                if (!d->st) throw std::runtime_error("Invalid struct declaration");
                if (functions.count(d->st->name) || structs.count(d->st->name) || enums.count(d->st->name) || statics.count(d->st->name)) {
                    throw std::runtime_error("Duplicate top-level name '" + d->st->name +
                                             "' at line " + std::to_string(d->st->line));
                }
                structs[d->st->name] = d->st.get();
                break;
            case DeclKind::Enum:
                if (!d->en) throw std::runtime_error("Invalid enum declaration");
                if (functions.count(d->en->name) || structs.count(d->en->name) || enums.count(d->en->name) || statics.count(d->en->name)) {
                    throw std::runtime_error("Duplicate top-level name '" + d->en->name +
                                             "' at line " + std::to_string(d->en->line));
                }
                enums[d->en->name] = d->en.get();
                break;
            case DeclKind::ComptimeGlobal:
                break;
            case DeclKind::Static:
                if (!d->staticDecl) throw std::runtime_error("Invalid static declaration");
                if (functions.count(d->staticDecl->name) || structs.count(d->staticDecl->name) ||
                    enums.count(d->staticDecl->name) || statics.count(d->staticDecl->name)) {
                    throw std::runtime_error("Duplicate top-level name '" + d->staticDecl->name +
                                             "' at line " + std::to_string(d->staticDecl->line));
                }
                statics[d->staticDecl->name] = d->staticDecl.get();
                break;
        }
    }

    {
        std::set<std::string> allNames;
        for (const auto& d : program.decls) {
            std::string name;
            if (d->kind == DeclKind::Function) name = d->fn->name;
            else if (d->kind == DeclKind::Struct) name = d->st->name;
            else if (d->kind == DeclKind::Enum) name = d->en->name;
            else if (d->kind == DeclKind::Static) name = d->staticDecl->name;
            else continue;
            if (!allNames.insert(name).second) throw std::runtime_error("Duplicate top-level name '" + name + "'");
        }
    }

    for (const auto& [_, st] : structs) checkStructDecl(*st);
    for (const auto& [_, en] : enums) checkEnumDecl(*en);
    for (const auto& [_, fn] : functions) checkFunctionSignature(*fn);
    for (const auto& [_, st] : statics) checkStaticDecl(*st);
}

void TypeChecker::checkProgram(Program& program) {
    ComptimeEvaluator evaluator(symbols);
    for (auto& d : program.decls) {
        if (d->kind == DeclKind::ComptimeGlobal) {
            evaluator.evaluateAndDeclare(d->comptimeGlobal.get());
        }
    }
    registerTopLevel(program);

    // Borrow-return provenance is interprocedural: a caller must know which
    // parameter a callee's returned reference/view is tied to before the caller
    // itself is checked. Check the static call graph depth-first so callees are
    // analyzed before their callers, including forward declarations in source.
    // Recursive cycles are still legal for ordinary functions; a borrow-return
    // cycle simply has no provenance until the language grows a recursive lifetime
    // model, so we conservatively leave that cycle unannotated.
    std::map<std::string, int> visitState;
    std::function<void(const Expr*)> visitExpr;
    std::function<void(const Stmt*)> visitStmt;
    std::function<void(FunctionDecl*)> visitFunction;

    visitExpr = [&](const Expr* expr) {
        if (!expr) return;
        if (expr->kind == ExprKind::Call && expr->callee &&
            expr->callee->kind == ExprKind::Identifier) {
            auto it = functions.find(expr->callee->strValue);
            if (it != functions.end()) visitFunction(it->second);
        }
        visitExpr(expr->left.get());
        visitExpr(expr->right.get());
        visitExpr(expr->callee.get());
        visitExpr(expr->target.get());
        visitExpr(expr->value.get());
        visitExpr(expr->arena.get());
        for (const auto& arg : expr->args) visitExpr(arg.get());
        for (const auto& field : expr->fields) visitExpr(field.second.get());
    };

    visitStmt = [&](const Stmt* stmt) {
        if (!stmt) return;
        visitExpr(stmt->expr.get());
        visitExpr(stmt->assignValue.get());
        visitExpr(stmt->iterable.get());
        visitExpr(stmt->guardCondition.get());
        if (stmt->guardBody) visitStmt(stmt->guardBody.get());
        for (const auto& child : stmt->body) visitStmt(child.get());
        for (const auto& child : stmt->elseBody) visitStmt(child.get());
        visitExpr(stmt->comptimeValue.get());
    };

    visitFunction = [&](FunctionDecl* fn) {
        if (!fn) return;
        const auto it = visitState.find(fn->name);
        if (it != visitState.end()) {
            if (it->second == 2 || it->second == 1) return;
        }
        visitState[fn->name] = 1;
        for (const auto& stmt : fn->body) visitStmt(stmt.get());
        if (!fn->isExtern) checkFunction(*fn);
        visitState[fn->name] = 2;
    };

    for (auto& d : program.decls) {
        if (d->kind == DeclKind::Function) visitFunction(d->fn.get());
    }
}

bool TypeChecker::alwaysTerminates(const Stmt* stmt) const {
    if (!stmt) return false;
    if (stmt->kind == StmtKind::Return || stmt->kind == StmtKind::Break || stmt->kind == StmtKind::Continue) return true;
    if (stmt->kind == StmtKind::UnsafeBlock) {
        if (stmt->body.empty()) return false;
        return alwaysTerminates(stmt->body.back().get());
    }
    if (stmt->kind != StmtKind::If || stmt->elseBody.empty()) return false;
    if (stmt->body.empty() || stmt->elseBody.empty()) return false;
    return alwaysTerminates(stmt->body.back().get()) && alwaysTerminates(stmt->elseBody.back().get());
}

bool TypeChecker::checkBlock(const std::vector<std::unique_ptr<Stmt>>& body) {
    bool term = false;
    for (const auto& s : body) {
        if (term) {
            throw std::runtime_error("Unreachable statement at line " + std::to_string(s->line));
        }
        ++currentStmtSerial;
        checkStmt(s.get());
        releaseBorrowsAtLastUse();
        term = alwaysTerminates(s.get());
    }
    return term;
}

std::unique_ptr<TypeNode> TypeChecker::inferExprType(const Expr* expr) {
    if (!expr) throw std::runtime_error("Type error: null expression");
    if (expr->checkedType) return cloneType(expr->checkedType.get());
    return checkExpr(expr, nullptr);
}

void TypeChecker::checkFunction(FunctionDecl& fn) {
    if (fn.isExtern) return;
    symbols.pushScope();
    auto savedReturn = std::move(currentReturnType);
    const int savedLoopDepth = loopDepth;
    const int savedUnsafeDepth = unsafeDepth;
    const std::string savedFunctionName = std::move(currentFunctionName);
    currentReturnType = cloneType(fn.returnType.get());
    currentFunctionName = fn.name;
    loopDepth = 0;
    unsafeDepth = fn.isUnsafe ? 1 : 0;
    currentStmtSerial = 0;
    lastUseByName.clear();
    std::size_t useSerial = 0;
    collectBlockUses(fn.body, useSerial);

    try {
        for (auto& p : fn.params) {
            Symbol s;
            s.name = p.name;
            s.type = p.type.get();
            s.declaredLine = fn.line;
            s.isParameter = true;
            s.type->origin = stable::memory::StorageOrigin{
                stable::memory::StorageOriginKind::Parameter, p.name};
            symbols.declare(p.name, s);
        }

        const bool term = checkBlock(fn.body);
        if (!term && !isVoid(currentReturnType.get())) {
            throw std::runtime_error("Function '" + fn.name + "' may fall through without returning " +
                                     typeToString(currentReturnType.get()));
        }

        popCheckedScope();
        currentReturnType = std::move(savedReturn);
        currentFunctionName = std::move(savedFunctionName);
        loopDepth = savedLoopDepth;
        unsafeDepth = savedUnsafeDepth;
    } catch (...) {
        popCheckedScope();
        currentReturnType = std::move(savedReturn);
        currentFunctionName = std::move(savedFunctionName);
        loopDepth = savedLoopDepth;
        unsafeDepth = savedUnsafeDepth;
        throw;
    }
}

[[noreturn]] void TypeChecker::error(const Expr* expr, const std::string& message) const {
    const int line = expr ? expr->line : 0;
    const int column = expr ? expr->column : 1;
    throw std::runtime_error("Type error at " + std::to_string(line) + ":" +
                             std::to_string(column) + ": " + message);
}

bool TypeChecker::literalFits(const Expr* expr, const TypeNode* type) const {
    if (!expr || !type || expr->kind != ExprKind::IntLit || !isInteger(type)) return true;
    try {
        const std::uint64_t value = parseUnsignedLiteralText(expr->strValue);
        int bits = integerBits(type->name);
        if (type->name == "isize" || type->name == "usize") bits = pointerBits;
        if (isSignedInteger(type->name)) {
            if (bits >= 64) return true;
            const std::uint64_t max = (std::uint64_t{1} << (bits - 1)) - 1;
            return value <= max;
        }
        if (bits >= 64) return true;
        return value < (std::uint64_t{1} << bits);
    } catch (...) {
        return false;
    }
}

namespace {

std::string borrowRoot(const Expr* expr) {
    if (!expr) return {};
    if (expr->kind == ExprKind::Identifier) return expr->strValue;
    if (expr->kind == ExprKind::FieldAccess) return borrowRoot(expr->target.get());
    if (expr->kind == ExprKind::Index) return borrowRoot(expr->target.get());
    if (expr->kind == ExprKind::Slice) return borrowRoot(expr->target.get());
    return {};
}

} // namespace

stable::memory::StorageOrigin TypeChecker::inferOrigin(const Symbol* owner, const std::string& ownerName) {
    if (owner && owner->type && owner->type->origin.valid()) return owner->type->origin;
    stable::memory::StorageOrigin origin;
    origin.kind = owner && owner->isParameter
        ? stable::memory::StorageOriginKind::Parameter
        : stable::memory::StorageOriginKind::Local;
    origin.binding = ownerName;
    return origin;
}

std::optional<stable::memory::BorrowRecord> TypeChecker::registerBorrow(
    Symbol* owner, const TypeNode* borrowedType, const std::string& ownerName) const {
    if (!owner || ownerName.empty()) return std::nullopt;
    const bool exclusive = borrowedType &&
        (borrowedType->isMutable || borrowedType->name == "EditView");
    if (exclusive && owner->isConst) {
        throw std::runtime_error("cannot create a mutable borrow of const value '" + ownerName + "'");
    }

    stable::memory::BorrowRecord record;
    record.mode = exclusive ? stable::memory::BorrowMode::Exclusive
                            : stable::memory::BorrowMode::Shared;
    record.origin = inferOrigin(owner, ownerName);
    record.ownerBinding = ownerName;
    record.ownerId = owner->bindingId;
    if (!stable::memory::mayBeginBorrow(owner->memory, record.mode)) {
        if (exclusive) {
            throw std::runtime_error("cannot create a mutable borrow of '" + ownerName + "' while it is already borrowed");
        }
        throw std::runtime_error("cannot create a shared borrow of '" + ownerName + "' while it has a mutable borrow");
    }
    stable::memory::beginBorrow(owner->memory, record);
    return record;
}

void TypeChecker::releaseBorrow(Symbol* borrower) {
    if (!borrower || !borrower->memory.borrow) return;
    const auto record = *borrower->memory.borrow;
    Symbol* owner = record.ownerId != 0 ? symbols.resolveBinding(record.ownerId)
                                        : symbols.resolve(record.ownerBinding);
    if (owner) stable::memory::endBorrow(owner->memory, record);
    borrower->memory.borrow.reset();
}

void TypeChecker::releaseBorrowsAtLastUse() {
    auto* scope = symbols.currentScope();
    if (!scope) return;
    std::vector<std::string> toRelease;
    for (const auto& [name, sym] : scope->entries()) {
        if (!sym.memory.borrow) continue;
        const auto it = lastUseByName.find(name);
        // A newly-created borrow with no later identifier use expires at the end
        // of its defining statement. This is the empty-use case of last-use analysis.
        if (it == lastUseByName.end() || it->second <= currentStmtSerial) {
            toRelease.push_back(name);
        }
    }
    for (const auto& name : toRelease) {
        if (auto* borrower = scope->resolveLocal(name)) releaseBorrow(borrower);
    }
}

void TypeChecker::collectExprUses(const Expr* expr, std::size_t serial) {
    if (!expr) return;
    if (expr->kind == ExprKind::Identifier) {
        lastUseByName[expr->strValue] = serial;
    }
    collectExprUses(expr->left.get(), serial);
    collectExprUses(expr->right.get(), serial);
    collectExprUses(expr->callee.get(), serial);
    collectExprUses(expr->target.get(), serial);
    collectExprUses(expr->value.get(), serial);
    collectExprUses(expr->arena.get(), serial);
    for (const auto& arg : expr->args) collectExprUses(arg.get(), serial);
    for (const auto& field : expr->fields) collectExprUses(field.second.get(), serial);
}

void TypeChecker::collectStmtUses(const Stmt* stmt, std::size_t& serial) {
    if (!stmt) return;
    ++serial;
    // A plain identifier on the left side of a declaration/assignment is a definition,
    // not a use. Field/index lvalues still count their contained owner identifiers.
    if (!((stmt->kind == StmtKind::Assign || stmt->kind == StmtKind::ConstAssign) &&
          stmt->expr && stmt->expr->kind == ExprKind::Identifier)) {
        collectExprUses(stmt->expr.get(), serial);
    }
    collectExprUses(stmt->assignValue.get(), serial);
    collectExprUses(stmt->iterable.get(), serial);
    collectExprUses(stmt->guardCondition.get(), serial);
    collectExprUses(stmt->comptimeValue.get(), serial);
    collectBlockUses(stmt->body, serial);
    collectBlockUses(stmt->elseBody, serial);
    collectStmtUses(stmt->guardBody.get(), serial);
}

void TypeChecker::collectBlockUses(const std::vector<std::unique_ptr<Stmt>>& body, std::size_t& serial) {
    for (const auto& stmt : body) collectStmtUses(stmt.get(), serial);
}




void TypeChecker::popCheckedScope() {
    auto* scope = symbols.currentScope();
    if (scope) {
        std::vector<std::string> toRelease;
        for (const auto& [name, sym] : scope->entries()) {
            if (sym.memory.borrow) toRelease.push_back(name);
        }
        for (const auto& name : toRelease) {
            if (auto* borrower = scope->resolveLocal(name)) releaseBorrow(borrower);
        }
    }
    symbols.popScope();
}

std::unique_ptr<TypeNode> TypeChecker::checkExpr(const Expr* expr, const TypeNode* expected) {
    if (!expr) throw std::runtime_error("Type error: null expression");

    auto finish = [&](std::unique_ptr<TypeNode> type) {
        auto* mutableExpr = const_cast<Expr*>(expr);
        mutableExpr->implicitOptionalWrap = false;
        if (expected && expected->isOptional && type && !type->isOptional) {
            auto payload = cloneType(expected);
            payload->isOptional = false;
            if (compatible(type.get(), payload.get())) {
                mutableExpr->implicitOptionalWrap = true;
                type = cloneType(expected);
            }
        }
        expr->checkedType = cloneType(type.get());
        return type;
    };

    switch (expr->kind) {
        case ExprKind::IntLit: {
            // Atomic[T] stores exactly the payload T in memory. Allow the literal
            // shorthand `a: Atomic[u64] = 0` and preserve the contextual atomic type
            // on the expression for the backend.
            if (expected && isAtomic(expected) && expected->generics.size() == 1) {
                const auto* payload = expected->generics[0].get();
                if (!literalFits(expr, payload)) {
                    error(expr, "integer literal does not fit " + typeToString(payload));
                }
                return finish(cloneType(expected));
            }
            auto t = makeType(expected && isInteger(expected) ? expected->name : "i32");
            if (expected && isInteger(expected) && !literalFits(expr, expected)) {
                error(expr, "integer literal does not fit " + typeToString(expected));
            }
            if (expected && !compatible(t.get(), expected)) {
                error(expr, "expected " + typeToString(expected) + ", got " + typeToString(t.get()));
            }
            return finish(std::move(t));
        }
        case ExprKind::FloatLit: {
            auto t = makeType(expected && (expected->name == "f32" || expected->name == "f64") ? expected->name : "f64");
            if (expected && !compatible(t.get(), expected)) {
                error(expr, "expected " + typeToString(expected) + ", got " + typeToString(t.get()));
            }
            return finish(std::move(t));
        }
        case ExprKind::StringLit: {
            auto t = makeType("string");
            if (expected && !compatible(t.get(), expected)) error(expr, "expected " + typeToString(expected) + ", got string");
            return finish(std::move(t));
        }
        case ExprKind::BoolLit: {
            auto t = makeType("bool");
            if (expected && !compatible(t.get(), expected)) error(expr, "expected " + typeToString(expected) + ", got bool");
            return finish(std::move(t));
        }
        case ExprKind::NoneLit: {
            if (expr->strValue == "null") {
                if (!expected || !expected->isRawPointer) error(expr, "'null' requires a raw-pointer expected type");
                return finish(cloneType(expected));
            }
            if (!expected || !expected->isOptional) error(expr, "'none' requires an optional expected type");
            return finish(cloneType(expected));
        }
        case ExprKind::Identifier: {
            Symbol* s = symbols.resolve(expr->strValue);
            if (!s) {
                auto fit = functions.find(expr->strValue);
                if (fit != functions.end()) {
                    auto fnType = makeType("fn");
                    fnType->isUnsafeFunction = fit->second->isUnsafe;
                    for (const auto& p : fit->second->params) fnType->generics.push_back(cloneType(p.type.get()));
                    fnType->generics.push_back(cloneType(fit->second->returnType.get()));
                    if (!expected || !compatible(fnType.get(), expected)) {
                        error(expr, "function '" + expr->strValue + "' can only be used as a compatible function-pointer value");
                    }
                    return finish(std::move(fnType));
                }
                error(expr, "unknown identifier '" + expr->strValue + "'");
            }
            if (s->isComptime) {
                // Comptime globals never populate Symbol::type (they aren't real
                // storage), so they must be typed from their ComptimeValue instead,
                // the same way a literal adapts to the expected context.
                std::unique_ptr<TypeNode> ct;
                if (std::holds_alternative<std::int64_t>(*s->comptimeValue)) {
                    ct = makeType(expected && isInteger(expected) ? expected->name : "i32");
                } else if (std::holds_alternative<double>(*s->comptimeValue)) {
                    ct = makeType(expected && (expected->name == "f32" || expected->name == "f64") ? expected->name : "f64");
                } else if (std::holds_alternative<bool>(*s->comptimeValue)) {
                    ct = makeType("bool");
                } else {
                    ct = makeType("string");
                }
                if (expected && !compatible(ct.get(), expected)) {
                    error(expr, "expected " + typeToString(expected) + ", got " + typeToString(ct.get()));
                }
                return finish(std::move(ct));
            }
            if (!s->type || !s->type->isRawPointer) {
                if (s->memory.isMoved()) error(expr, "use of moved value '" + expr->strValue + "'");
                if (s->memory.hasExclusiveBorrow()) {
                    error(expr, "cannot access '" + expr->strValue + "' while an exclusive borrow is active");
                }
            }
            auto t = cloneType(s->type);
            // References are transparent in value contexts. Keeping the reference
            // when no expected type is supplied preserves explicit reference assignment,
            // while arithmetic/returns/arguments naturally read the referenced value.
            if (expected && !expected->isReference && t->isReference) {
                auto valueType = cloneType(t.get());
                valueType->isReference = false;
                valueType->isMutable = false;
                valueType->origin = {};
                if (!isCopyType(valueType.get())) {
                    error(expr, "cannot read borrowed non-copy value as an owned value");
                }
                t = std::move(valueType);
            }
            if (expected && !compatible(t.get(), expected)) error(expr, "expected " + typeToString(expected) + ", got " + typeToString(t.get()));
            return finish(std::move(t));
        }
        case ExprKind::UnaryOp: {
            // Unary numeric operators preserve the surrounding numeric context.
            // This is essential for typed literals such as `x: f32 = -0.9` and
            // `x: i64 = -918273645`: the operand must be checked against the
            // declared/expected type rather than first defaulting to i32/f64.
            if (expr->op == "*") {
                if (unsafeDepth <= 0) error(expr, "raw-pointer dereference requires an unsafe block");
                auto pointer = inferExprType(expr->value.get());
                if (!pointer->isRawPointer) error(expr, "unary '*' requires a raw pointer");
                if (pointer->name == "void") error(expr, "cannot dereference a void raw pointer");
                auto pointee = cloneType(pointer.get());
                pointee->isRawPointer = false;
                pointee->isMutable = false;
                if (expected && !compatible(pointee.get(), expected)) error(expr, "expected " + typeToString(expected) + ", got " + typeToString(pointee.get()));
                return finish(std::move(pointee));
            }
            auto inner = ((expr->op == "+" || expr->op == "-") && expected && isNumeric(expected))
                ? checkExpr(expr->value.get(), expected)
                : inferExprType(expr->value.get());
            if (expr->op == "&raw") {
                if (unsafeDepth <= 0) error(expr, "raw address creation requires an unsafe block");
                if (expr->value->kind != ExprKind::Identifier && expr->value->kind != ExprKind::FieldAccess && expr->value->kind != ExprKind::Index && expr->value->kind != ExprKind::UnaryOp) {
                    error(expr, "raw address creation requires an addressable value");
                }
                auto raw = cloneType(inner.get());
                raw->isReference = false;
                raw->isRawPointer = true;
                raw->isMutable = true;
                raw->origin = {};
                return finish(std::move(raw));
            }
            if (expr->op == "&" || expr->op == "&mut") {
                if (expr->value->kind != ExprKind::Identifier && expr->value->kind != ExprKind::FieldAccess && expr->value->kind != ExprKind::Index) {
                    error(expr, "reference creation requires an addressable value");
                }
                const std::string ownerName = borrowRoot(expr->value.get());
                if (ownerName.empty()) error(expr, "could not determine the owner of this reference");
                auto* owner = symbols.resolve(ownerName);
                if (!owner || owner->memory.isMoved()) error(expr, "cannot borrow an unknown or moved value");
                const bool mut = expr->op == "&mut";
                const auto mode = mut ? stable::memory::BorrowMode::Exclusive : stable::memory::BorrowMode::Shared;
                if (!stable::memory::mayBeginBorrow(owner->memory, mode)) {
                    error(expr, mut ? "cannot create &mut while the value is already borrowed"
                                    : "cannot create a shared reference while the value has a mutable borrow");
                }
                inner->isReference = true;
                inner->isMutable = mut;
                inner->origin = inferOrigin(owner, ownerName);
                if (expected && !compatible(inner.get(), expected)) {
                    error(expr, "expected " + typeToString(expected) + ", got " + typeToString(inner.get()));
                }
                return finish(std::move(inner));
            }
            if (expr->op == "+" || expr->op == "-") {
                if (!isNumeric(inner.get())) error(expr, "unary '" + expr->op + "' requires a numeric operand");
                if (expected && !compatible(inner.get(), expected)) {
                    error(expr, "expected " + typeToString(expected) + ", got " + typeToString(inner.get()));
                }
                return finish(std::move(inner));
            }
            if (expr->op == "!") {
                if (!isBoolean(inner.get())) error(expr, "'!' requires bool");
                return finish(makeType("bool"));
            }
            if (expr->op == "~") {
                if (!isInteger(inner.get())) error(expr, "'~' requires an integer operand");
                return finish(std::move(inner));
            }
            error(expr, "unknown unary operator '" + expr->op + "'");
        }
        case ExprKind::BinaryOp: {
            auto t = checkBinaryOp(expr, expected);
            if (expected && !compatible(t.get(), expected)) error(expr, "expected " + typeToString(expected) + ", got " + typeToString(t.get()));
            return finish(std::move(t));
        }
        case ExprKind::Reference: {
            auto inner = inferExprType(expr->value.get());
            inner->isReference = true;
            return finish(std::move(inner));
        }
        case ExprKind::ArenaAlloc: {
            auto value = inferExprType(expr->value.get());
            if (!value->isArray || value->fixedArraySize) {
                error(expr, "'in arena' currently requires a dynamic array literal such as []T in arena");
            }
            if (expr->arena->kind != ExprKind::Identifier) error(expr, "arena target of 'in' must be a simple variable");
            auto arenaExprType = inferExprType(expr->arena.get());
            Symbol* arena = symbols.resolve(expr->arena->strValue);
            if (!arena || !arena->type || arena->type->name != "Arena" || !arenaExprType || arenaExprType->name != "Arena") {
                error(expr, "'" + expr->arena->strValue + "' is not an Arena");
            }
            if (arena->memory.isMoved()) error(expr, "cannot allocate from moved Arena '" + expr->arena->strValue + "'");
            // An arena-backed value inherits the arena binding's lifetime. A function
            // parameter can legally carry the result across the callee boundary because
            // the caller owns the region; a function-local Arena cannot escape. Encode the
            // former as a parameter origin so the general short-lived-origin machinery
            // does not mistake it for a local region.
            const auto arenaOriginKind = arena->isParameter
                ? stable::memory::StorageOriginKind::Parameter
                : stable::memory::StorageOriginKind::Arena;
            value->origin = stable::memory::StorageOrigin{arenaOriginKind, expr->arena->strValue};
            mergeNestedOrigins(value.get(), expr->value->checkedType.get());
            if (expected && !compatible(value.get(), expected)) error(expr, "expected " + typeToString(expected) + ", got " + typeToString(value.get()));
            return finish(std::move(value));
        }
        case ExprKind::ArrayLit: {
            std::unique_ptr<TypeNode> t;

            // An expected fixed-array type supplies the element type directly. This must
            // happen before checking elements so an owning identifier is moved exactly
            // once, rather than first being inferred and then checked a second time.
            std::unique_ptr<TypeNode> elementType;
            std::unique_ptr<TypeNode> expectedArray;
            if (expected && expected->isOptional) {
                expectedArray = cloneType(expected);
                expectedArray->isOptional = false;
            } else if (expected) {
                expectedArray = cloneType(expected);
            }

            if (expectedArray && expectedArray->isArray && expectedArray->fixedArraySize) {
                if (expr->args.size() != *expectedArray->fixedArraySize) {
                    error(expr, "array literal has " + std::to_string(expr->args.size()) +
                                " elements but " + std::to_string(*expectedArray->fixedArraySize) + " were required");
                }
                elementType = cloneType(expectedArray.get());
                elementType->isArray = false;
                elementType->fixedArraySize.reset();
                elementType->isOptional = false;
                elementType->origin = {};
                elementType->isReference = false;
                elementType->isMutable = false;
                t = std::move(expectedArray);
            } else {
                if (!expr->elementTypeName.empty()) {
                    t = makeType(expr->elementTypeName, true);
                    validateType(t.get(), expr->line);
                } else {
                    if (expr->args.empty()) error(expr, "cannot infer the element type of an empty array");
                    auto el = inferExprType(expr->args.front().get());
                    el->isArray = true;
                    t = std::move(el);
                }
                elementType = cloneType(t.get());
                elementType->isArray = false;
                elementType->fixedArraySize.reset();
                elementType->isOptional = false;
                elementType->origin = {};
                elementType->isReference = false;
                elementType->isMutable = false;
            }

            // Constructing an owning aggregate consumes an owned identifier, just like
            // passing it to an owning function parameter.
            for (const auto& arg : expr->args) {
                auto argType = checkExpr(arg.get(), elementType.get());
                mergeNestedOrigins(t.get(), argType.get());
                rejectBorrowedOwnerProjection(arg.get(), elementType.get(), "array literal");
                if (arg->kind == ExprKind::Identifier && !isCopyType(elementType.get())) {
                    if (auto* source = symbols.resolve(arg->strValue)) {
                        rejectMoveWhileBorrowed(source, arg.get(), "array literal");
                        source->memory.markMoved();
                    }
                }
            }

            if (expected && !compatible(t.get(), expected)) {
                error(expr, "expected " + typeToString(expected) + ", got " + typeToString(t.get()));
            }
            return finish(std::move(t));
        }
        case ExprKind::StructLit: {
            auto t = checkStructLiteral(expr);
            if (expected && !compatible(t.get(), expected)) error(expr, "expected " + typeToString(expected) + ", got " + typeToString(t.get()));
            return finish(std::move(t));
        }
        case ExprKind::FieldAccess: {
            auto t = checkFieldAccess(expr);
            // Provenance needs to follow a field only when that field can itself
            // carry a borrow/region dependency. Avoid re-resolving nominal enum/struct
            // qualified constants such as `Color.White`, whose target is a type name.
            if (isView(t.get()) || t->isReference || requiresOwnershipTransfer(t.get())) {
                const bool qualifiedTypeConstant =
                    expr->target && expr->target->kind == ExprKind::Identifier &&
                    !symbols.resolve(expr->target->strValue) &&
                    (structs.count(expr->target->strValue) || enums.count(expr->target->strValue));
                if (!qualifiedTypeConstant) {
                    auto target = inferExprType(expr->target.get());
                    mergeNestedOrigins(t.get(), target.get());
                }
            }
            if (expected && !compatible(t.get(), expected)) error(expr, "expected " + typeToString(expected) + ", got " + typeToString(t.get()));
            return finish(std::move(t));
        }
        case ExprKind::Call:
            return finish(checkCall(expr, expected));
        case ExprKind::OkLit:
        case ExprKind::ErrLit:
            return finish(checkResultLiteral(expr, expected));
        case ExprKind::IsMatch:
            return finish(checkIsMatch(expr));
        case ExprKind::Cast: {
            auto t = checkCast(expr);
            if (expected && !compatible(t.get(), expected)) error(expr, "expected " + typeToString(expected) + ", got " + typeToString(t.get()));
            return finish(std::move(t));
        }
        case ExprKind::Slice: {
            auto target = inferExprType(expr->target.get());
            if (!target->isArray && !isView(target.get())) {
                error(expr, "slicing requires an array or View");
            }

            std::unique_ptr<TypeNode> element;
            stable::memory::StorageOrigin sourceOrigin = target->origin;
            if (target->isArray) {
                element = cloneType(target.get());
                element->isArray = false;
                element->fixedArraySize.reset();
                element->origin = {};
                element->isReference = false;
                element->isMutable = false;
                if (expr->target->kind == ExprKind::Identifier) {
                    if (auto* source = symbols.resolve(expr->target->strValue)) sourceOrigin = inferOrigin(source, expr->target->strValue);
                }
            } else {
                element = cloneType(target->generics.at(0).get());
                element->origin = {};
                sourceOrigin = target->origin;
            }

            const auto checkBound = [&](const Expr* bound, std::uint64_t limit, const char* label) {
                if (!bound || bound->kind != ExprKind::IntLit) return;
                try {
                    const auto value = parseUnsignedLiteralText(bound->strValue);
                    if (value > limit) error(bound, std::string(label) + " bound is outside the source range");
                } catch (const std::exception&) {
                    error(bound, "invalid slice bound");
                }
            };

            // Slice bounds are full expressions, not just optional integer literals.
            // Type-check them here so nested expressions such as `a[0:a.len()]`
            // carry semantic types into later code generation.
            auto boundType = makeType("usize");
            if (!expr->args.empty() && expr->args[0]) checkExpr(expr->args[0].get(), boundType.get());
            if (expr->args.size() > 1 && expr->args[1]) checkExpr(expr->args[1].get(), boundType.get());

            if (target->fixedArraySize) {
                const std::uint64_t limit = *target->fixedArraySize;
                if (!expr->args.empty()) checkBound(expr->args[0].get(), limit, "slice lower");
                if (expr->args.size() > 1) checkBound(expr->args[1].get(), limit, "slice upper");
                if (expr->args.size() == 2 && expr->args[0] && expr->args[1] &&
                    expr->args[0]->kind == ExprKind::IntLit && expr->args[1]->kind == ExprKind::IntLit) {
                    const auto lo = parseUnsignedLiteralText(expr->args[0]->strValue);
                    const auto hi = parseUnsignedLiteralText(expr->args[1]->strValue);
                    if (lo > hi) error(expr, "slice lower bound exceeds upper bound");
                }
            }

            bool mutableView = false;
            if (isView(target.get()) && target->name == "EditView") mutableView = true;
            if (expected && expected->name == "EditView") mutableView = true;
            auto view = makeType(mutableView ? "EditView" : "View");
            view->generics.push_back(std::move(element));
            if (!sourceOrigin.valid()) {
                const auto ownerName = borrowRoot(expr->target.get());
                if (!ownerName.empty()) {
                    if (auto* owner = symbols.resolve(ownerName)) sourceOrigin = inferOrigin(owner, ownerName);
                }
            }
            view->origin = sourceOrigin;
            return finish(std::move(view));
        }

        case ExprKind::Index: {
            auto source = inferExprType(expr->target.get());
            auto target = cloneType(source.get());
            if (source->isRawPointer) {
                if (unsafeDepth <= 0) error(expr, "raw pointer indexing requires an unsafe block");
                if (source->name == "void") error(expr, "cannot index a void raw pointer");
                if (expr->args.size() != 1) error(expr, "raw pointer indexing requires one index");
                auto index = inferExprType(expr->args[0].get());
                if (!isInteger(index.get())) error(expr, "raw pointer index must be an integer");
                target->isRawPointer = false;
                target->isMutable = false;
                target->origin = {};
                if (expected && !compatible(target.get(), expected)) error(expr, "expected " + typeToString(expected) + ", got " + typeToString(target.get()));
                return finish(std::move(target));
            }
            if (source->name == "string" && !source->isArray && !source->isReference) {
                auto index = inferExprType(expr->args.at(0).get());
                if (!isInteger(index.get())) error(expr, "string index must be an integer");
                target = makeType("u8");
                if (expected && !compatible(target.get(), expected)) error(expr, "expected " + typeToString(expected) + ", got u8");
                return finish(std::move(target));
            }
            if (!source->isArray && !isView(source.get())) error(expr, "indexing requires an array, View, or string");
            // Indexing through a reference borrows the container, but the element
            // expression is a value by default. The element should not retain the
            // container's reference qualifier.
            if (source->isReference) {
                target->isReference = false;
                target->isMutable = false;
                target->origin = {};
            }
            auto index = inferExprType(expr->args.at(0).get());
            if (!isInteger(index.get())) error(expr, "array index must be an integer");
            if (source->fixedArraySize && expr->args[0]->kind == ExprKind::IntLit) {
                const auto idx = parseUnsignedLiteralText(expr->args[0]->strValue);
                if (idx >= *source->fixedArraySize) error(expr, "fixed-array index is out of bounds");
            }
            if (isView(source.get())) target = cloneType(source->generics.at(0).get());
            else {
                target->isArray = false;
                target->fixedArraySize.reset();
                target->origin = {};
            }
            // Preserve provenance captured by an aggregate when the extracted
            // element can itself carry that dependency onward. Scalar values are
            // independent copies, while views/references and non-copy aggregates
            // retain the conservative nested provenance.
            if (isView(target.get()) || target->isReference || requiresOwnershipTransfer(target.get())) {
                for (const auto& origin : source->nestedOrigins) rememberOrigin(target->nestedOrigins, origin);
            }
            if (expected && !compatible(target.get(), expected)) error(expr, "expected " + typeToString(expected) + ", got " + typeToString(target.get()));
            return finish(std::move(target));
        }
    }

    throw std::runtime_error("Unreachable expression kind");
}

std::unique_ptr<TypeNode> TypeChecker::checkBinaryOp(const Expr* expr, const TypeNode* expected) {
    const std::string& op = expr->op;
    const bool expectedNumeric = expected && isNumeric(expected);
    const bool arithmeticLike = op == "+" || op == "-" || op == "*" || op == "/" || op == "%" ||
                                op == "|" || op == "&" || op == "^" || op == "<<" || op == ">>";
    auto left = arithmeticLike && expectedNumeric ? checkExpr(expr->left.get(), expected)
                                                  : inferExprType(expr->left.get());

    if (left->isRawPointer && (op == "+" || op == "-")) {
        if (unsafeDepth <= 0) error(expr, "raw pointer arithmetic requires an unsafe block");
        auto right = inferExprType(expr->right.get());
        if (isInteger(right.get())) return cloneType(left.get());
        error(expr, "raw pointer arithmetic requires an integer offset");
    }
    if (left->isRawPointer && (op == "+") ) {
        auto right = inferExprType(expr->right.get());
        if (isInteger(right.get())) return cloneType(left.get());
    }
    if ((op == "+") && isInteger(left.get())) {
        auto right = inferExprType(expr->right.get());
        if (right->isRawPointer) {
            if (unsafeDepth <= 0) error(expr, "raw pointer arithmetic requires an unsafe block");
            return cloneType(right.get());
        }
    }
    auto right = checkExpr(expr->right.get(), left.get());


    if (op == "&&" || op == "||") {
        if (!isBoolean(left.get()) || !isBoolean(right.get())) error(expr, "logical operators require bool operands");
        return makeType("bool");
    }

    if (op == "==" || op == "!=") {
        if (!compatible(left.get(), right.get()) && !compatible(right.get(), left.get())) {
            error(expr, "comparison operands have incompatible types");
        }
        return makeType("bool");
    }

    if (op == "<" || op == ">" || op == "<=" || op == ">=") {
        if (left->isRawPointer && right->isRawPointer && typesEqual(left.get(), right.get())) {
            if (unsafeDepth <= 0) error(expr, "raw pointer ordering comparison requires an unsafe block");
            return makeType("bool");
        }
        if (!isNumeric(left.get()) || !isNumeric(right.get()) || !typesEqual(left.get(), right.get())) {
            error(expr, "ordering comparison requires two values of the same numeric type");
        }
        return makeType("bool");
    }

    if (op == "|" || op == "&" || op == "^" || op == "<<" || op == ">>" || op == "%") {
        if (!isInteger(left.get()) || !isInteger(right.get()) || !typesEqual(left.get(), right.get())) {
            error(expr, "bitwise/shift/modulo operators require integer operands of the same type");
        }
        return cloneType(left.get());
    }

    if (op == "+" || op == "-" || op == "*" || op == "/") {
        if (!isNumeric(left.get()) || !isNumeric(right.get()) || !typesEqual(left.get(), right.get())) {
            error(expr, "arithmetic operands must have the same numeric type");
        }
        return cloneType(left.get());
    }

    error(expr, "unknown binary operator '" + op + "'");
}

std::unique_ptr<TypeNode> TypeChecker::checkCall(const Expr* expr, const TypeNode* expected) {
    // Small, explicit runtime surface used by the standard library and bootstrap
    // compiler. These are compiler-known primitives, not magic overload resolution.
    if (expr->callee->kind == ExprKind::Identifier) {
        const std::string& name = expr->callee->strValue;
        if (name == "allocAligned") {
            if (expr->args.size() != 2) error(expr, "allocAligned() takes byte-count and alignment");
            checkExpr(expr->args[0].get(), makeType("usize").get());
            checkExpr(expr->args[1].get(), makeType("usize").get());
            auto r = makeType("u8"); r->isRawPointer = true; r->isMutable = true;
            return r;
        }
        if (name == "deallocAligned") {
            if (expr->args.size() != 1) error(expr, "deallocAligned() takes exactly one pointer argument");
            auto p = makeType("u8"); p->isRawPointer = true; p->isMutable = true;
            checkExpr(expr->args[0].get(), p.get());
            return makeType("void");
        }
        if (name == "sizeOf" || name == "alignOf") {
            if (expr->args.size() != 1) error(expr, name + "() takes exactly one value whose type is inspected");
            auto t = inferExprType(expr->args[0].get());
            if (isVoid(t.get())) error(expr, name + "() cannot inspect void");
            return makeType("usize");
        }
        if (name == "offsetOf") {
            if (expr->args.size() != 2 || expr->args[1]->kind != ExprKind::StringLit) error(expr, "offsetOf() takes a struct value and a field-name string");
            auto t = inferExprType(expr->args[0].get());
            if (t->isReference) t->isReference = false;
            if (t->isArray || !structs.count(t->name)) error(expr, "offsetOf() requires a struct value");
            const auto fieldName = expr->args[1]->strValue;
            bool found = false;
            for (const auto& f : structs.at(t->name)->fields) if (f.name == fieldName) { found = true; break; }
            if (!found) error(expr, "struct '" + t->name + "' has no field '" + fieldName + "'");
            return makeType("usize");
        }
        if (name == "alloc") {
            if (expr->args.size() != 1) error(expr, "alloc() takes exactly one byte-count argument");
            checkExpr(expr->args[0].get(), makeType("usize").get());
            auto r = makeType("u8"); r->isRawPointer = true; r->isMutable = true;
            return r;
        }
        if (name == "realloc") {
            if (expr->args.size() != 2) error(expr, "realloc() takes pointer and byte-count arguments");
            auto p = makeType("u8"); p->isRawPointer = true; p->isMutable = true;
            checkExpr(expr->args[0].get(), p.get());
            checkExpr(expr->args[1].get(), makeType("usize").get());
            auto r = makeType("u8"); r->isRawPointer = true; r->isMutable = true;
            return r;
        }
        if (name == "ptrDiff") {
            if (expr->args.size() != 2) error(expr, "ptrDiff() takes two raw pointers");
            auto a = inferExprType(expr->args[0].get());
            auto b = inferExprType(expr->args[1].get());
            if (!a->isRawPointer || !b->isRawPointer || !typesEqual(a.get(), b.get())) {
                error(expr, "ptrDiff() requires two raw pointers of the same pointee type");
            }
            if (unsafeDepth <= 0) error(expr, "ptrDiff() requires an unsafe block");
            return makeType("isize");
        }
        if (name == "stackAlloc") {
            if (unsafeDepth <= 0) error(expr, "stackAlloc() requires an unsafe block");
            if (expr->args.size() < 1 || expr->args.size() > 2) error(expr, "stackAlloc() takes a byte count and optional constant alignment");
            checkExpr(expr->args[0].get(), makeType("usize").get());
            if (expr->args.size() == 2) {
                if (expr->args[1]->kind != ExprKind::IntLit) error(expr, "stackAlloc() alignment must be an integer literal");
                const auto alignment = parseUnsignedLiteralText(expr->args[1]->strValue);
                if (alignment == 0 || (alignment & (alignment - 1)) != 0) error(expr, "stackAlloc() alignment must be a positive power of two");
            }
            auto r = makeType("u8"); r->isRawPointer = true; r->isMutable = true;
            return r;
        }
        if (name == "assume") {
            if (expr->args.size() != 1) error(expr, "assume() takes exactly one bool condition");
            checkExpr(expr->args[0].get(), makeType("bool").get());
            return makeType("void");
        }
        if (name == "trap") {
            if (!expr->args.empty()) error(expr, "trap() takes no arguments");
            return makeType("void");
        }
        if (name == "dealloc") {
            if (expr->args.size() != 1) error(expr, "dealloc() takes exactly one pointer argument");
            auto p = makeType("u8"); p->isRawPointer = true; p->isMutable = true;
            checkExpr(expr->args[0].get(), p.get());
            auto r = makeType("void");
            return r;
        }
        if (name == "volatileLoad") {
            if (expr->args.size() != 1) error(expr, "volatileLoad() takes exactly one raw pointer");
            auto p = inferExprType(expr->args[0].get());
            if (!p->isRawPointer) error(expr, "volatileLoad() requires a raw pointer");
            auto r = cloneType(p.get()); r->isRawPointer = false; r->isMutable = false;
            return r;
        }
        if (name == "volatileStore") {
            if (expr->args.size() != 2) error(expr, "volatileStore() takes pointer and value arguments");
            auto p = inferExprType(expr->args[0].get());
            if (!p->isRawPointer || !p->isMutable) error(expr, "volatileStore() requires a mutable raw pointer");
            auto r = cloneType(p.get()); r->isRawPointer = false; r->isMutable = false;
            checkExpr(expr->args[1].get(), r.get());
            return makeType("void");
        }
        if (name == "unalignedLoad") {
            if (expr->args.size() != 1) error(expr, "unalignedLoad() takes exactly one raw pointer");
            auto p = inferExprType(expr->args[0].get());
            if (!p->isRawPointer || p->name == "void") error(expr, "unalignedLoad() requires a non-void raw pointer");
            auto r = cloneType(p.get()); r->isRawPointer = false; r->isMutable = false;
            return r;
        }
        if (name == "unalignedStore") {
            if (expr->args.size() != 2) error(expr, "unalignedStore() takes pointer and value arguments");
            auto p = inferExprType(expr->args[0].get());
            if (!p->isRawPointer || !p->isMutable || p->name == "void") error(expr, "unalignedStore() requires a mutable non-void raw pointer");
            auto r = cloneType(p.get()); r->isRawPointer = false; r->isMutable = false;
            checkExpr(expr->args[1].get(), r.get());
            return makeType("void");
        }
        if (name == "memset") {
            if (expr->args.size() != 3) error(expr, "memset() takes pointer, byte, and size");
            auto p = makeType("u8"); p->isRawPointer = true; p->isMutable = true;
            checkExpr(expr->args[0].get(), p.get()); checkExpr(expr->args[1].get(), makeType("u8").get()); checkExpr(expr->args[2].get(), makeType("usize").get());
            return cloneType(p.get());
        }
        if (name == "memcpy") {
            if (expr->args.size() != 3) error(expr, "memcpy() takes destination, source, and size");
            auto dp = makeType("u8"); dp->isRawPointer = true; dp->isMutable = true;
            auto sp = makeType("u8"); sp->isRawPointer = true; sp->isMutable = false;
            checkExpr(expr->args[0].get(), dp.get()); checkExpr(expr->args[1].get(), sp.get()); checkExpr(expr->args[2].get(), makeType("usize").get());
            return cloneType(dp.get());
        }
        if (name == "memmove") {
            if (expr->args.size() != 3) error(expr, "memmove() takes destination, source, and size");
            auto dp = makeType("u8"); dp->isRawPointer = true; dp->isMutable = true;
            auto sp = makeType("u8"); sp->isRawPointer = true; dp->isMutable = true;
            checkExpr(expr->args[0].get(), dp.get()); checkExpr(expr->args[1].get(), sp.get()); checkExpr(expr->args[2].get(), makeType("usize").get());
            return cloneType(dp.get());
        }
        if (name == "memcmp") {
            if (expr->args.size() != 3) error(expr, "memcmp() takes two pointers and a size");
            auto dp = makeType("u8"); dp->isRawPointer = true; dp->isMutable = false;
            checkExpr(expr->args[0].get(), dp.get()); checkExpr(expr->args[1].get(), dp.get()); checkExpr(expr->args[2].get(), makeType("usize").get());
            return makeType("i32");
        }
        if (name == "atomicFence") {
            if (expr->args.size() != 1 || expr->args[0]->kind != ExprKind::StringLit) {
                error(expr, "atomicFence() requires one memory-order string literal");
            }
            const auto& order = expr->args[0]->strValue;
            if (order != "relaxed" && order != "acquire" && order != "release" && order != "acq_rel" && order != "seq_cst") {
                error(expr, "invalid atomic memory order '" + order + "'");
            }
            return makeType("void");
        }
        if (name == "compilerFence") {
            if (!expr->args.empty()) error(expr, "compilerFence() takes no arguments");
            return makeType("void");
        }
        if (name == "unreachable") {
            if (!expr->args.empty()) error(expr, "unreachable() takes no arguments");
            return makeType("void");
        }
        if (name == "asm" || name == "asmI64" || name == "asmI32" || name == "asmPtr") {
            if (expr->args.size() < 2) error(expr, "asm() requires template and constraint strings");
            checkExpr(expr->args[0].get(), makeType("string").get()); checkExpr(expr->args[1].get(), makeType("string").get());
            for (std::size_t i=2;i<expr->args.size();++i) {
                auto operand = inferExprType(expr->args[i].get());
                if (!isInteger(operand.get()) && !operand->isRawPointer && !operand->isReference) error(expr, "asm operands must be integers, references, or raw pointers");
            }
            if (unsafeDepth <= 0) error(expr, "inline assembly requires an unsafe block");
            if (name == "asm") return makeType("void");
            if (name == "asmI32") return makeType("i32");
            if (name == "asmPtr") {
                auto p = makeType("u8"); p->isRawPointer = true; p->isMutable = true; return p;
            }
            return makeType("i64");
        }

        if (name == "readFile") {
            if (expr->args.size() != 1) error(expr, "readFile() takes exactly one argument");
            checkExpr(expr->args[0].get(), makeType("string").get());
            auto result = makeType("u8", true);
            if (expected && !compatible(result.get(), expected)) error(expr, "expected " + typeToString(expected) + ", got " + typeToString(result.get()));
            return result;
        }
        if (name == "writeStdout") {
            if (expr->args.size() != 1) error(expr, "writeStdout() takes exactly one argument");
            checkExpr(expr->args[0].get(), makeType("string").get());
            auto result = makeType("void");
            if (expected && !compatible(result.get(), expected)) error(expr, "expected " + typeToString(expected) + ", got void");
            return result;
        }
        if (name == "writeRaw") {
            if (expr->args.size() != 1) error(expr, "writeRaw() takes exactly one argument");
            checkExpr(expr->args[0].get(), makeType("string").get());
            auto result = makeType("void");
            if (expected && !compatible(result.get(), expected)) error(expr, "expected " + typeToString(expected) + ", got void");
            return result;
        }
        if (name == "writeIntRaw") {
            if (expr->args.size() != 1) error(expr, "writeIntRaw() takes exactly one argument");
            checkExpr(expr->args[0].get(), makeType("i64").get());
            auto result = makeType("void");
            if (expected && !compatible(result.get(), expected)) error(expr, "expected " + typeToString(expected) + ", got void");
            return result;
        }
        if (name == "writeByteRaw") {
            if (expr->args.size() != 1) error(expr, "writeByteRaw() takes exactly one argument");
            checkExpr(expr->args[0].get(), makeType("i64").get());
            auto result = makeType("void");
            if (expected && !compatible(result.get(), expected)) error(expr, "expected " + typeToString(expected) + ", got void");
            return result;
        }
        if (name == "print") {
            if (expr->args.size() != 1) error(expr, "print() takes exactly one argument");
            auto value = inferExprType(expr->args[0].get());
            const bool printable =
                value->name == "string" || value->name == "bool" ||
                isInteger(value.get()) || isFloatName(value->name);
            if (!printable || value->isArray || value->isOptional || value->isReference) {
                error(expr, "print() currently supports string, bool, integer, and floating-point values");
            }
            auto result = makeType("void");
            if (expected && !compatible(result.get(), expected)) error(expr, "expected " + typeToString(expected) + ", got void");
            return result;
        }
        if (name == "printInt") {
            if (expr->args.size() != 1) error(expr, "printInt() takes exactly one argument");
            checkExpr(expr->args[0].get(), makeType("i64").get());
            auto result = makeType("void");
            if (expected && !compatible(result.get(), expected)) error(expr, "expected " + typeToString(expected) + ", got void");
            return result;
        }
        if (name == "stringLen") {
            if (expr->args.size() != 1) error(expr, "stringLen() takes exactly one argument");
            checkExpr(expr->args[0].get(), makeType("string").get());
            auto result = makeType("usize");
            if (expected && !compatible(result.get(), expected)) error(expr, "expected " + typeToString(expected) + ", got " + typeToString(result.get()));
            return result;
        }
        if (name == "getEnv") {
            if (expr->args.size() != 1) error(expr, "getEnv() takes exactly one argument");
            checkExpr(expr->args[0].get(), makeType("string").get());
            auto result = makeType("string");
            if (expected && !compatible(result.get(), expected)) error(expr, "expected " + typeToString(expected) + ", got " + typeToString(result.get()));
            return result;
        }
    }

    // Built-in region constructor. It is deliberately a language primitive rather
    // than a library call so the compiler knows the resulting ownership boundary.
    if (expr->callee->kind == ExprKind::FieldAccess &&
        expr->callee->target->kind == ExprKind::Identifier &&
        expr->callee->target->strValue == "Arena" &&
        expr->callee->field == "create") {
        if (expr->args.size() != 1) error(expr, "Arena.create() takes exactly one size argument");
        auto sizeType = checkExpr(expr->args[0].get(), makeType("usize").get());
        if (!isInteger(sizeType.get())) error(expr, "Arena.create() size must be an integer");
        if (expr->args[0]->kind == ExprKind::IntLit) {
            try {
                const auto value = parseUnsignedLiteralText(expr->args[0]->strValue);
                if (value == 0) error(expr, "Arena.create() size must be positive");
            } catch (...) {
                error(expr, "invalid Arena.create() size");
            }
        }
        return makeType("Arena");
    }

    if (expr->callee->kind == ExprKind::FieldAccess) {
        // Qualified runtime namespaces such as Stdin.readLine() have no value
        // symbol to infer. Only inspect the target for string methods when it is
        // an actual value expression.
        const auto* targetExpr = expr->callee->target.get();
        const bool inferableTarget = targetExpr->kind != ExprKind::Identifier || symbols.resolve(targetExpr->strValue);
        if (inferableTarget) {
            const auto target = inferExprType(targetExpr);
            if (target && target->name == "string" && !target->isArray && !target->isReference) {
                const std::string member = expr->callee->field;
                if (member == "startsWith" || member == "equals") {
                    if (expr->args.size() != 1) error(expr, "string comparison method takes one string argument");
                    checkExpr(expr->args[0].get(), makeType("string").get());
                    return makeType("bool");
                }
                if (member == "parseU64At") {
                    if (expr->args.size() != 1) error(expr, "string.parseU64At() takes one index argument");
                    checkExpr(expr->args[0].get(), makeType("usize").get());
                    return makeType("u64");
                }
            }
        }
    }

    if (expr->callee->kind == ExprKind::FieldAccess && expr->callee->target &&
        expr->callee->target->kind == ExprKind::Identifier) {
        const std::string ns = expr->callee->target->strValue;
        const std::string member = expr->callee->field;
        if (ns == "Game") {
            if (isWebTarget()) error(expr, "Game namespace requires a native target; browser games should use Web/WebGL APIs");
            const auto need = [&](std::size_t n, const std::string& what) { if (expr->args.size() != n) error(expr, what + " has wrong arity"); };
            const auto str = [&](std::size_t i) { checkExpr(expr->args[i].get(), makeType("string").get()); };
            const auto i32 = [&](std::size_t i) { checkExpr(expr->args[i].get(), makeType("i32").get()); };
            const auto u32 = [&](std::size_t i) { checkExpr(expr->args[i].get(), makeType("u32").get()); };
            const auto win = [&](std::size_t i) { checkExpr(expr->args[i].get(), makeType("GameWindow").get()); };
            const auto ren = [&](std::size_t i) { checkExpr(expr->args[i].get(), makeType("GameRenderer").get()); };
            const auto tex = [&](std::size_t i) { checkExpr(expr->args[i].get(), makeType("GameTexture").get()); };
            const auto aud = [&](std::size_t i) { checkExpr(expr->args[i].get(), makeType("GameAudio").get()); };
            auto rawU8 = [&](std::size_t i, bool mut) { auto t=makeType("u8"); t->isRawPointer=true; t->isMutable=mut; checkExpr(expr->args[i].get(),t.get()); if(unsafeDepth<=0) error(expr,"Game raw buffer operations require an unsafe block"); };
            if (member=="createWindow") { need(4,"Game.createWindow"); str(0); i32(1); i32(2); u32(3); return makeType("GameWindow"); }
            if (member=="destroyWindow") { need(1,"Game.destroyWindow"); win(0); return makeType("void"); }
            if (member=="poll") { need(1,"Game.poll"); win(0); return makeType("i32"); }
            if (member=="shouldClose") { need(1,"Game.shouldClose"); win(0); return makeType("bool"); }
            if (member=="requestClose") { need(1,"Game.requestClose"); win(0); return makeType("void"); }
            if (member=="setTitle") { need(2,"Game.setTitle"); win(0); str(1); return makeType("void"); }
            if (member=="width" || member=="height") { need(1,"Game."+member); win(0); return makeType("i32"); }
            if (member=="setVSync") { need(2,"Game.setVSync"); win(0); checkExpr(expr->args[1].get(),makeType("bool").get()); return makeType("bool"); }
            if (member=="makeGLContext") { need(1,"Game.makeGLContext"); win(0); return makeType("bool"); }
            if (member=="present") { need(1,"Game.present"); win(0); return makeType("void"); }
            if (member=="windowFlags") { need(4,"Game.windowFlags"); for(int i=0;i<4;++i) checkExpr(expr->args[i].get(),makeType("bool").get()); return makeType("u32"); }
            if (member=="rendererFlags") { need(2,"Game.rendererFlags"); checkExpr(expr->args[0].get(),makeType("bool").get()); checkExpr(expr->args[1].get(),makeType("bool").get()); return makeType("u32"); }
            if (member=="createRenderer") { need(2,"Game.createRenderer"); win(0); u32(1); return makeType("GameRenderer"); }
            if (member=="destroyRenderer") { need(1,"Game.destroyRenderer"); ren(0); return makeType("void"); }
            if (member=="setDrawColor") { need(5,"Game.setDrawColor"); ren(0); for(int i=1;i<5;++i) checkExpr(expr->args[i].get(),makeType("u8").get()); return makeType("bool"); }
            if (member=="clear") { need(1,"Game.clear"); ren(0); return makeType("bool"); }
            if (member=="drawLine") { need(5,"Game.drawLine"); ren(0); for(int i=1;i<5;++i) i32(i); return makeType("bool"); }
            if (member=="fillRect") { need(5,"Game.fillRect"); ren(0); for(int i=1;i<5;++i) i32(i); return makeType("bool"); }
            if (member=="presentRenderer") { need(1,"Game.presentRenderer"); ren(0); return makeType("void"); }
            if (member=="createTexture") { need(5,"Game.createTexture"); ren(0); u32(1); u32(2); i32(3); i32(4); return makeType("GameTexture"); }
            if (member=="updateTexture") { need(3,"Game.updateTexture"); tex(0); rawU8(1,false); i32(2); return makeType("bool"); }
            if (member=="copyTexture") { need(5,"Game.copyTexture"); ren(0); tex(1); i32(2); i32(3); i32(4); return makeType("bool"); }
            if (member=="destroyTexture") { need(1,"Game.destroyTexture"); tex(0); return makeType("void"); }
            if (member=="eventType" || member=="eventCode" || member=="eventX" || member=="eventY") { need(0,"Game."+member); return makeType("i32"); }
            if (member=="eventText") { need(0,"Game.eventText"); return makeType("string"); }
            if (member=="keyDown") { need(1,"Game.keyDown"); i32(0); return makeType("bool"); }
            if (member=="mouseButtonDown") { need(1,"Game.mouseButtonDown"); i32(0); return makeType("bool"); }
            if (member=="mouseX" || member=="mouseY") { need(0,"Game."+member); return makeType("i32"); }
            if (member=="controllerConnected") { need(1,"Game.controllerConnected"); i32(0); return makeType("bool"); }
            if (member=="controllerAxis") { need(2,"Game.controllerAxis"); i32(0); i32(1); return makeType("f32"); }
            if (member=="controllerButtonDown") { need(2,"Game.controllerButtonDown"); i32(0); i32(1); return makeType("bool"); }
            if (member=="audioOpen") { need(3,"Game.audioOpen"); i32(0); i32(1); i32(2); return makeType("GameAudio"); }
            if (member=="audioWrite") { need(3,"Game.audioWrite"); aud(0); rawU8(1,false); checkExpr(expr->args[2].get(),makeType("usize").get()); return makeType("isize"); }
            if (member=="audioQueued") { need(1,"Game.audioQueued"); aud(0); return makeType("usize"); }
            if (member=="audioPause") { need(2,"Game.audioPause"); aud(0); checkExpr(expr->args[1].get(),makeType("bool").get()); return makeType("void"); }
            if (member=="audioClose") { need(1,"Game.audioClose"); aud(0); return makeType("void"); }
            if (member=="timeNanos") { need(0,"Game.timeNanos"); return makeType("i64"); }
            if (member=="deltaSeconds") { need(0,"Game.deltaSeconds"); return makeType("f64"); }
            if (member=="sleepNanos") { need(1,"Game.sleepNanos"); checkExpr(expr->args[0].get(),makeType("i64").get()); return makeType("void"); }
            error(expr,"unknown Game method '"+member+"'");
        }
        if (ns == "Graphics") {
            if (isWebTarget()) error(expr, "Graphics namespace requires a native target; browser rendering uses Web APIs");
            const auto need = [&](std::size_t n, const std::string& what) { if (expr->args.size() != n) error(expr, what + " has wrong arity"); };
            const auto str = [&](std::size_t i) { checkExpr(expr->args[i].get(),makeType("string").get()); };
            const auto u32 = [&](std::size_t i) { checkExpr(expr->args[i].get(),makeType("u32").get()); };
            const auto i32 = [&](std::size_t i) { checkExpr(expr->args[i].get(),makeType("i32").get()); };
            const auto f32 = [&](std::size_t i) { checkExpr(expr->args[i].get(),makeType("f32").get()); };
            auto raw = [&](std::size_t i, const char* elem, bool mut) { auto t=makeType(elem); t->isRawPointer=true; t->isMutable=mut; checkExpr(expr->args[i].get(),t.get()); if(unsafeDepth<=0) error(expr,"Graphics raw pointer operation requires an unsafe block"); };
            if(member=="available"){need(1,"Graphics.available");str(0);return makeType("bool");}
            if(member=="backend"){need(0,"Graphics.backend");return makeType("string");}
            if(member=="loadProc"){need(2,"Graphics.loadProc");str(0);str(1);if(unsafeDepth<=0)error(expr,"Graphics.loadProc requires an unsafe block");auto t=makeType("u8");t->isRawPointer=true;return t;}
            if(member=="glClearColor"){need(4,"Graphics.glClearColor");for(int i=0;i<4;++i)f32(i);return makeType("void");}
            if(member=="glClear"||member=="glEnable"||member=="glDisable"||member=="glBindBuffer"||member=="glUseProgram"||member=="glDrawArrays"||member=="glBindVertexArray"||member=="glEnableVertexAttribArray"||member=="glDeleteShader"||member=="glDeleteProgram"){std::size_t n=(member=="glClear"||member=="glEnable"||member=="glDisable")?1:(member=="glBindBuffer"?2:(member=="glUseProgram"||member=="glDeleteShader"||member=="glDeleteProgram"?1:3));need(n,"Graphics."+member);for(std::size_t i=0;i<n;++i)u32(i);if(member=="glDrawArrays"){i32(1);i32(2);}return makeType("void");}
            if(member=="glViewport"){need(4,"Graphics.glViewport");for(int i=0;i<4;++i)i32(i);return makeType("void");}
            if(member=="glGenBuffers"||member=="glGenVertexArrays"){need(2,"Graphics."+member);i32(0);raw(1,"u32",true);return makeType("void");}
            if(member=="glBufferData"){need(4,"Graphics.glBufferData");u32(0);checkExpr(expr->args[1].get(),makeType("usize").get());raw(2,"u8",false);u32(3);return makeType("void");}
            if(member=="glCreateShader"){need(1,"Graphics.glCreateShader");u32(0);return makeType("u32");}
            if(member=="glShaderSource"){need(2,"Graphics.glShaderSource");u32(0);str(1);return makeType("void");}
            if(member=="glCompileShader"){need(1,"Graphics.glCompileShader");u32(0);return makeType("void");}
            if(member=="glShaderStatus"){need(1,"Graphics.glShaderStatus");u32(0);return makeType("bool");}
            if(member=="glShaderLog"){need(1,"Graphics.glShaderLog");u32(0);return makeType("Buffer");}
            if(member=="glCreateProgram"){need(0,"Graphics.glCreateProgram");return makeType("u32");}
            if(member=="glAttachShader"){need(2,"Graphics.glAttachShader");u32(0);u32(1);return makeType("void");}
            if(member=="glLinkProgram"){need(1,"Graphics.glLinkProgram");u32(0);return makeType("void");}
            if(member=="glProgramStatus"){need(1,"Graphics.glProgramStatus");u32(0);return makeType("bool");}
            if(member=="glProgramLog"){need(1,"Graphics.glProgramLog");u32(0);return makeType("Buffer");}
            if(member=="glVertexAttribPointer"){need(6,"Graphics.glVertexAttribPointer");i32(0);i32(1);u32(2);checkExpr(expr->args[3].get(),makeType("bool").get());i32(4);checkExpr(expr->args[5].get(),makeType("usize").get());return makeType("void");}
            if(member=="glGetError"){need(0,"Graphics.glGetError");return makeType("u32");}
            if(member=="glDeleteBuffers"||member=="glDeleteVertexArrays"){need(2,"Graphics."+member);i32(0);raw(1,"u32",false);return makeType("void");}
            error(expr,"unknown Graphics method '"+member+"'");
        }
        if (ns == "Mobile") {
            if (!isMobileTarget()) error(expr, "Mobile namespace requires an Android or iOS target");
            const auto need = [&](std::size_t n, const std::string& what) {
                if (expr->args.size() != n) error(expr, what + " takes " + std::to_string(n) + (n == 1 ? " argument" : " arguments"));
            };
            const auto str = [&](std::size_t i, const std::string& label) { checkExpr(expr->args[i].get(), makeType("string").get()); (void)label; };
            if (member == "log") { need(1, "Mobile.log()"); str(0, "message"); return makeType("void"); }
            if (member == "platform" || member == "osVersion" || member == "appDataPath" || member == "documentsPath" || member == "cachePath") { need(0, "Mobile." + member + "()"); return makeType("string"); }
            if (member == "isSimulator" || member == "cameraAvailable" || member == "locationAvailable" || member == "bluetoothAvailable") { need(0, "Mobile." + member + "()"); return makeType("bool"); }
            if (member == "screenWidth" || member == "screenHeight" || member == "safeAreaTop" || member == "safeAreaBottom" || member == "safeAreaLeft" || member == "safeAreaRight") { need(0, "Mobile." + member + "()"); return makeType("i32"); }
            if (member == "deviceScale") { need(0, "Mobile.deviceScale()"); return makeType("f64"); }
            if (member == "openUrl" || member == "clipboardSet") { need(1, "Mobile." + member + "()"); str(0, "value"); return makeType("bool"); }
            if (member == "clipboardGet") { need(0, "Mobile.clipboardGet()"); return makeType("Buffer"); }
            if (member == "vibrate") { need(1, "Mobile.vibrate()"); checkExpr(expr->args[0].get(), makeType("u32").get()); return makeType("bool"); }
            if (member == "requestPermission") { need(1, "Mobile.requestPermission()"); str(0, "permission"); return makeType("i32"); }
            error(expr, "unknown Mobile method '" + member + "'");
        }
        if (ns == "Web") {
            if (!isWebTarget()) error(expr, "Web namespace requires a wasm32 Web target; compile with --web");
            const auto requireArgs = [&](std::size_t n, const std::string& label) {
                if (expr->args.size() != n) error(expr, label + " takes " + std::to_string(n) + (n == 1 ? " argument" : " arguments"));
            };
            const auto checkStringAt = [&](std::size_t index, const std::string& label) {
                checkExpr(expr->args[index].get(), makeType("string").get());
                if (expr->args[index]->kind == ExprKind::StringLit &&
                    expr->args[index]->strValue.empty()) error(expr, label + " must not be empty");
            };
            const auto checkCallback = [&](std::size_t index, const std::string& label, const std::vector<std::string>& wanted) {
                checkStringAt(index, label);
                const auto* a = expr->args[index].get();
                if (a->kind != ExprKind::StringLit) return;
                auto it = functions.find(a->strValue);
                if (it == functions.end() || it->second->isExtern) error(expr, label + " names a Stable function that does not exist");
                const auto* fn = it->second;
                if (fn->params.size() != wanted.size()) error(expr, label + " function has the wrong arity");
                for (std::size_t i = 0; i < wanted.size(); ++i) {
                    if (typeToString(fn->params[i].type.get()) != wanted[i]) error(expr, label + " function parameter " + std::to_string(i) + " must be " + wanted[i]);
                }
                if (typeToString(fn->returnType.get()) != "void") error(expr, label + " function must return void");
            };
            if (member == "log" || member == "warn" || member == "error") {
                requireArgs(1, "Web." + member + "()"); checkStringAt(0, "message"); return makeType("void");
            }
            if (member == "nowMs" || member == "random") {
                requireArgs(0, "Web." + member + "()"); return makeType("f64");
            }
            if (member == "setText" || member == "setHtml") {
                requireArgs(2, "Web." + member + "()"); checkStringAt(0, "selector"); checkStringAt(1, "text"); return makeType("i32");
            }
            if (member == "setAttribute") {
                requireArgs(3, "Web.setAttribute()"); checkStringAt(0, "selector"); checkStringAt(1, "name"); checkStringAt(2, "value"); return makeType("i32");
            }
            if (member == "addClass" || member == "removeClass") {
                requireArgs(2, "Web." + member + "()"); checkStringAt(0, "selector"); checkStringAt(1, "class"); return makeType("i32");
            }
            if (member == "remove" || member == "queryCount" || member == "focus") {
                requireArgs(1, "Web." + member + "()"); checkStringAt(0, "selector"); return makeType("i32");
            }
            if (member == "setTimeout") {
                requireArgs(2, "Web.setTimeout()"); checkCallback(0, "timeout callback", {}); checkExpr(expr->args[1].get(), makeType("u32").get()); return makeType("i32");
            }
            if (member == "clearTimeout" || member == "cancelAnimationFrame" || member == "removeEventListener") {
                requireArgs(1, "Web." + member + "()"); checkExpr(expr->args[0].get(), makeType("i32").get()); return makeType("void");
            }
            if (member == "requestAnimationFrame") {
                requireArgs(1, "Web.requestAnimationFrame()"); checkCallback(0, "animation-frame callback", {"f64"}); return makeType("i32");
            }
            if (member == "queueMicrotask") {
                requireArgs(1, "Web.queueMicrotask()"); checkCallback(0, "microtask callback", {}); return makeType("i32");
            }
            if (member == "addEventListener") {
                requireArgs(3, "Web.addEventListener()"); checkStringAt(0, "selector"); checkStringAt(1, "event"); checkCallback(2, "event callback", {}); return makeType("i32");
            }
            if (member == "fetchText") {
                requireArgs(2, "Web.fetchText()"); checkStringAt(0, "URL"); checkCallback(1, "fetch callback", {"i32", "*mut u8", "usize"}); return makeType("i32");
            }
            if (member == "freeBuffer") {
                requireArgs(1, "Web.freeBuffer()"); auto t = inferExprType(expr->args[0].get()); if (!t->isRawPointer) error(expr, "Web.freeBuffer() requires a raw pointer buffer"); return makeType("void");
            }
            error(expr, "unknown Web method '" + member + "'");
        }
        if (isWebTarget() && (ns == "Stdin" || ns == "Clock" || ns == "Thread" || ns == "Cpu")) {
            error(expr, ns + " runtime is not available in browser WebAssembly; use the Web namespace");
        }
        if (ns == "Stdin") {
            if (member == "hasInput") {
                if (!expr->args.empty()) error(expr, "Stdin.hasInput() takes no arguments");
                return makeType("bool");
            }
            if (member == "readLine") {
                if (!expr->args.empty()) error(expr, "Stdin.readLine() takes no arguments");
                return makeType("string");
            }
            error(expr, "unknown Stdin method '" + member + "'");
        }
        if (ns == "Clock") {
            if (member == "monotonicNanos") {
                if (!expr->args.empty()) error(expr, "Clock.monotonicNanos() takes no arguments");
                return makeType("u64");
            }
            if (member == "sleepNanos") {
                if (expr->args.size() != 1) error(expr, "Clock.sleepNanos() takes one argument");
                checkExpr(expr->args[0].get(), makeType("u64").get());
                return makeType("void");
            }
            if (member == "deadlineAfterNanos") {
                if (expr->args.size() != 1) error(expr, "Clock.deadlineAfterNanos() takes one argument");
                checkExpr(expr->args[0].get(), makeType("u64").get());
                return makeType("u64");
            }
            if (member == "expired") {
                if (expr->args.size() != 1) error(expr, "Clock.expired() takes one argument");
                checkExpr(expr->args[0].get(), makeType("u64").get());
                return makeType("bool");
            }
            if (member == "remainingNanos") {
                if (expr->args.size() != 1) error(expr, "Clock.remainingNanos() takes one argument");
                checkExpr(expr->args[0].get(), makeType("u64").get());
                return makeType("u64");
            }
            error(expr, "unknown Clock method '" + member + "'");
        }
        if (ns == "Cpu") {
            if (member == "hasAvx2" || member == "hasAvx512" || member == "hasSse42" || member == "hasBmi2" || member == "hasPopcnt") {
                if (!expr->args.empty()) error(expr, "CPU feature query takes no arguments");
                return makeType("bool");
            }
            if (member == "rdtsc") {
                if (!expr->args.empty()) error(expr, "Cpu.rdtsc() takes no arguments");
                return makeType("u64");
            }
            if (member == "popcount" || member == "ctz" || member == "clz" || member == "bswap") {
                if (expr->args.size() != 1) error(expr, "CPU integer intrinsic takes one argument");
                auto t = checkExpr(expr->args[0].get(), makeType("u64").get());
                if (!isInteger(t.get())) error(expr, "CPU integer intrinsic requires an integer");
                if (member == "popcount" || member == "ctz" || member == "clz") return makeType("u32");
                return t;
            }
            if (member == "pext" || member == "pdep") {
                if (expr->args.size() != 2) error(expr, "BMI2 intrinsic takes two arguments");
                checkExpr(expr->args[0].get(), makeType("u64").get());
                checkExpr(expr->args[1].get(), makeType("u64").get());
                return makeType("u64");
            }
            if (member == "loadV128" || member == "loadV256" || member == "loadV512") {
                if (expr->args.size() != 1) error(expr, "SIMD load takes one reference argument");
                auto t = inferExprType(expr->args[0].get());
                if (!t->isReference || t->name != "u64") error(expr, "SIMD load requires an explicit &u64 reference");
                return makeType(member == "loadV128" ? "v128" : member == "loadV256" ? "v256" : "v512");
            }
            if (member == "storeV128" || member == "storeV256" || member == "storeV512") {
                if (expr->args.size() != 2) error(expr, "SIMD store takes a vector and a mutable reference");
                const auto expectedVec = makeType(member == "storeV128" ? "v128" : member == "storeV256" ? "v256" : "v512");
                checkExpr(expr->args[0].get(), expectedVec.get());
                auto ptrType = inferExprType(expr->args[1].get());
                if (!ptrType->isReference || !ptrType->isMutable || ptrType->name != "u64") error(expr, "SIMD store requires an explicit &mut u64 reference");
                return makeType("void");
            }
            if (member == "zeroV128" || member == "zeroV256" || member == "zeroV512") {
                if (!expr->args.empty()) error(expr, "SIMD zero constructor takes no arguments");
                return makeType(member == "zeroV128" ? "v128" : member == "zeroV256" ? "v256" : "v512");
            }
            if (member == "prefetch") {
                if (expr->args.size() != 1) error(expr, "Cpu.prefetch() takes one reference argument");
                auto t = inferExprType(expr->args[0].get());
                if (!t->isReference) error(expr, "Cpu.prefetch() requires an explicit reference");
                return makeType("void");
            }
            error(expr, "unknown Cpu intrinsic '" + member + "'");
        }
        if (ns == "Thread") {
            if (member == "spawn") {
                if (expr->args.size() != 1 || expr->args[0]->kind != ExprKind::Identifier) {
                    error(expr, "Thread.spawn() requires one zero-argument function name");
                }
                auto it = functions.find(expr->args[0]->strValue);
                if (it == functions.end() || it->second->isExtern || !it->second->params.empty() || !isVoid(it->second->returnType.get())) {
                    error(expr, "Thread.spawn() target must be a Stable function with signature () void");
                }
                return makeType("Thread");
            }
            if (member == "hardwareConcurrency") {
                if (!expr->args.empty()) error(expr, "Thread.hardwareConcurrency() takes no arguments");
                return makeType("usize");
            }
            if (member == "yield") {
                if (!expr->args.empty()) error(expr, "Thread.yield() takes no arguments");
                return makeType("void");
            }
            error(expr, "unknown Thread method '" + member + "'");
        }
        if (isWebTarget() && (ns == "Net" || ns == "Poller" || ns == "Sync" || ns == "Mutex" || ns == "RwLock" || ns == "Condvar" || ns == "Semaphore" || ns == "Process" || ns == "Http" || ns == "Json" || ns == "Buffer" || ns == "Tensor" || ns == "Grad" || ns == "Autograd" || ns == "Accel")) {
            error(expr, ns + " backend runtime is not available in browser WebAssembly");
        }
        if (ns == "Net") {
            const auto need = [&](std::size_t n, const std::string& what) { if (expr->args.size() != n) error(expr, what + " has wrong arity"); };
            const auto str = [&](std::size_t i, const char* what) { checkExpr(expr->args[i].get(), makeType("string").get()); (void)what; };
            const auto socket = [&]() { auto t = makeType("Socket"); return t; };
            if (member == "tcpConnect") { need(3, "Net.tcpConnect"); str(0, "host"); checkExpr(expr->args[1].get(), makeType("u16").get()); checkExpr(expr->args[2].get(), makeType("i32").get()); return socket(); }
            if (member == "tcpListen") { need(3, "Net.tcpListen"); str(0, "host"); checkExpr(expr->args[1].get(), makeType("u16").get()); checkExpr(expr->args[2].get(), makeType("i32").get()); return socket(); }
            if (member == "accept") { need(1, "Net.accept"); checkExpr(expr->args[0].get(), makeType("Socket").get()); return socket(); }
            if (member == "close") { need(1, "Net.close"); checkExpr(expr->args[0].get(), makeType("Socket").get()); return makeType("void"); }
            if (member == "send") { need(3, "Net.send"); checkExpr(expr->args[0].get(), makeType("Socket").get()); auto p=makeType("u8"); p->isRawPointer=true; checkExpr(expr->args[1].get(), p.get()); checkExpr(expr->args[2].get(), makeType("usize").get()); return makeType("isize"); }
            if (member == "recv") { need(3, "Net.recv"); checkExpr(expr->args[0].get(), makeType("Socket").get()); auto p=makeType("u8"); p->isRawPointer=true; p->isMutable=true; checkExpr(expr->args[1].get(), p.get()); checkExpr(expr->args[2].get(), makeType("usize").get()); return makeType("isize"); }
            if (member == "sendString") { need(2, "Net.sendString"); checkExpr(expr->args[0].get(), makeType("Socket").get()); str(1, "text"); return makeType("isize"); }
            if (member == "setNonblocking" || member == "tcpNoDelay") { need(2, "Net." + member); checkExpr(expr->args[0].get(), makeType("Socket").get()); checkExpr(expr->args[1].get(), makeType("bool").get()); return makeType("bool"); }
            if (member == "poll") { need(3, "Net.poll"); checkExpr(expr->args[0].get(), makeType("Socket").get()); checkExpr(expr->args[1].get(), makeType("i32").get()); checkExpr(expr->args[2].get(), makeType("i32").get()); return makeType("i32"); }
            if (member == "shutdown") { need(2, "Net.shutdown"); checkExpr(expr->args[0].get(), makeType("Socket").get()); checkExpr(expr->args[1].get(), makeType("i32").get()); return makeType("i32"); }
            if (member == "lastError") { need(0, "Net.lastError"); return makeType("i32"); }
            if (member == "errorString") { need(0, "Net.errorString"); return makeType("string"); }
            if (member == "udpOpen") { need(0, "Net.udpOpen"); return socket(); }
            if (member == "udpBind") { need(3, "Net.udpBind"); checkExpr(expr->args[0].get(), makeType("Socket").get()); str(1, "host"); checkExpr(expr->args[2].get(), makeType("u16").get()); return makeType("bool"); }
            if (member == "udpSendTo") { need(5, "Net.udpSendTo"); checkExpr(expr->args[0].get(), makeType("Socket").get()); str(1, "host"); checkExpr(expr->args[2].get(), makeType("u16").get()); auto p=makeType("u8"); p->isRawPointer=true; checkExpr(expr->args[3].get(), p.get()); checkExpr(expr->args[4].get(), makeType("usize").get()); return makeType("isize"); }
            if (member == "udpRecv") { need(3, "Net.udpRecv"); checkExpr(expr->args[0].get(), makeType("Socket").get()); auto p=makeType("u8"); p->isRawPointer=true; p->isMutable=true; checkExpr(expr->args[1].get(), p.get()); checkExpr(expr->args[2].get(), makeType("usize").get()); return makeType("isize"); }
            if (member == "localPort" || member == "peerPort") { need(1, "Net." + member); checkExpr(expr->args[0].get(), makeType("Socket").get()); return makeType("u16"); }
            error(expr, "unknown Net method '" + member + "'");
        }
        if (ns == "Poller") {
            const auto need = [&](std::size_t n, const std::string& what) { if (expr->args.size()!=n) error(expr, what+" has wrong arity"); };
            if (member == "create") { need(0, "Poller.create"); return makeType("Poller"); }
            if (member == "add") { need(3, "Poller.add"); checkExpr(expr->args[0].get(), makeType("Poller").get()); checkExpr(expr->args[1].get(), makeType("Socket").get()); checkExpr(expr->args[2].get(), makeType("i32").get()); return makeType("bool"); }
            if (member == "remove") { need(2, "Poller.remove"); checkExpr(expr->args[0].get(), makeType("Poller").get()); checkExpr(expr->args[1].get(), makeType("Socket").get()); return makeType("bool"); }
            if (member == "wait") { need(2, "Poller.wait"); checkExpr(expr->args[0].get(), makeType("Poller").get()); checkExpr(expr->args[1].get(), makeType("i32").get()); return makeType("i32"); }
            if (member == "count") { need(1, "Poller.count"); checkExpr(expr->args[0].get(), makeType("Poller").get()); return makeType("i32"); }
            if (member == "eventSocket") { need(2, "Poller.eventSocket"); checkExpr(expr->args[0].get(), makeType("Poller").get()); checkExpr(expr->args[1].get(), makeType("i32").get()); return makeType("Socket"); }
            if (member == "eventMask") { need(2, "Poller.eventMask"); checkExpr(expr->args[0].get(), makeType("Poller").get()); checkExpr(expr->args[1].get(), makeType("i32").get()); return makeType("i32"); }
            if (member == "readEvents" || member == "writeEvents" || member == "errorEvents") { need(0, "Poller." + member); return makeType("i32"); }
            if (member == "destroy") { need(1, "Poller.destroy"); checkExpr(expr->args[0].get(), makeType("Poller").get()); return makeType("void"); }
            error(expr, "unknown Poller method '" + member + "'");
        }
        if (ns == "Mutex" || ns == "RwLock" || ns == "Condvar" || ns == "Semaphore") {
            const std::string ty = ns;
            const auto need = [&](std::size_t n) { if (expr->args.size()!=n) error(expr, ns + "." + member + " has wrong arity"); };
            if (member == "create") { need(ns == "Semaphore" ? 1 : 0); if (ns=="Semaphore") checkExpr(expr->args[0].get(), makeType("u32").get()); return makeType(ty); }
            if (ns == "Mutex") {
                if (member=="lock" || member=="unlock" || member=="destroy") { need(1); checkExpr(expr->args[0].get(), makeType(ty).get()); return makeType("void"); }
                if (member=="tryLock") { need(1); checkExpr(expr->args[0].get(), makeType(ty).get()); return makeType("bool"); }
            }
            if (ns == "RwLock") {
                if (member=="readLock" || member=="writeLock" || member=="destroy") { need(1); checkExpr(expr->args[0].get(), makeType(ty).get()); return makeType("void"); }
                if (member=="tryReadLock" || member=="tryWriteLock") { need(1); checkExpr(expr->args[0].get(), makeType(ty).get()); return makeType("bool"); }
                if (member=="unlock") { need(2); checkExpr(expr->args[0].get(), makeType(ty).get()); checkExpr(expr->args[1].get(), makeType("bool").get()); return makeType("void"); }
            }
            if (ns == "Condvar") {
                if (member=="wait") { need(3); checkExpr(expr->args[0].get(), makeType(ty).get()); checkExpr(expr->args[1].get(), makeType("Mutex").get()); checkExpr(expr->args[2].get(), makeType("i32").get()); return makeType("bool"); }
                if (member=="signal" || member=="broadcast" || member=="destroy") { need(1); checkExpr(expr->args[0].get(), makeType(ty).get()); return makeType("void"); }
            }
            if (ns == "Semaphore") {
                if (member=="wait") { need(2); checkExpr(expr->args[0].get(), makeType(ty).get()); checkExpr(expr->args[1].get(), makeType("i32").get()); return makeType("bool"); }
                if (member=="tryWait" || member=="post" || member=="destroy") { need(1); checkExpr(expr->args[0].get(), makeType(ty).get()); return member=="tryWait"?makeType("bool"):makeType("void"); }
            }
            error(expr, "unknown " + ns + " method '" + member + "'");
        }
        if (ns == "Tensor") {
            const auto need = [&](std::size_t n, const std::string& what){ if (expr->args.size() != n) error(expr, what + " has wrong arity"); };
            const auto usize = [&](std::size_t i){ checkExpr(expr->args[i].get(), makeType("usize").get()); };
            const auto f64 = [&](std::size_t i){ checkExpr(expr->args[i].get(), makeType("f64").get()); };
            const auto tensor = [&](std::size_t i){ checkExpr(expr->args[i].get(), makeType("Tensor").get()); };
            const auto raw = [&](std::size_t i, const char* element, bool mut){ auto t=makeType(element); t->isRawPointer=true; t->isMutable=mut; checkExpr(expr->args[i].get(),t.get()); };
            if (member=="zeros1" || member=="zeros2" || member=="zeros3" || member=="zeros4" || member=="ones1" || member=="ones2" || member=="ones3" || member=="ones4") {
                const int rank = member.back()-'0'; need((std::size_t)rank, "Tensor."+member); for(int i=0;i<rank;++i) usize((std::size_t)i); return makeType("Tensor");
            }
            if (member=="zerosF32" || member=="onesF32" || member=="zerosF64" || member=="onesF64") { need(1, "Tensor."+member); usize(0); return makeType("Tensor"); }
            if (member=="from1F32" || member=="from1F64") { need(2, "Tensor."+member); raw(0, (member.size() >= 3 && member.compare(member.size()-3, 3, "F64") == 0)?"f64":"f32", false); usize(1); if (unsafeDepth<=0) error(expr, "Tensor."+member+" requires an unsafe block"); return makeType("Tensor"); }
            if (member=="from2F32" || member=="from2F64") { need(3, "Tensor."+member); raw(0, (member.size() >= 3 && member.compare(member.size()-3, 3, "F64") == 0)?"f64":"f32", false); usize(1); usize(2); if (unsafeDepth<=0) error(expr, "Tensor."+member+" requires an unsafe block"); return makeType("Tensor"); }
            if (member=="clone") { need(1,"Tensor.clone"); tensor(0); return makeType("Tensor"); }
            if (member=="free") { need(1,"Tensor.free"); tensor(0); return makeType("void"); }
            if (member=="rank" || member=="len" || member=="dim" || member=="stride") { need(member=="dim"||member=="stride" ? 2 : 1, "Tensor."+member); tensor(0); if(member=="dim"||member=="stride") usize(1); return makeType("usize"); }
            if (member=="dtype" || member=="isContiguous") { need(1,"Tensor."+member); tensor(0); return makeType(member=="dtype"?"u32":"bool"); }
            if (member=="dataF32" || member=="dataF64") { need(1,"Tensor."+member); tensor(0); if(unsafeDepth<=0) error(expr,"Tensor."+member+" requires an unsafe block"); auto t=makeType((member.size() >= 3 && member.compare(member.size()-3, 3, "F64") == 0)?"f64":"f32"); t->isRawPointer=true; t->isMutable=true; return t; }
            if (member=="get1" || member=="get2" || member=="get3") { const auto n=(std::size_t)(member.back()-'0'); need(1+n,"Tensor."+member); tensor(0); for(std::size_t i=0;i<n;++i) usize(1+i); return makeType("f64"); }
            if (member=="set1" || member=="set2" || member=="set3") { const auto n=(std::size_t)(member.back()-'0'); need(2+n,"Tensor."+member); tensor(0); for(std::size_t i=0;i<n;++i) usize(1+i); f64(1+n); return makeType("void"); }
            if (member=="add" || member=="sub" || member=="mul" || member=="div" || member=="matmul") { need(2,"Tensor."+member); tensor(0); tensor(1); return makeType("Tensor"); }
            if (member=="scale") { need(2,"Tensor.scale"); tensor(0); f64(1); return makeType("Tensor"); }
            if (member=="relu" || member=="sigmoid" || member=="tanh" || member=="softmax") { need(member=="softmax"?2:1,"Tensor."+member); tensor(0); if(member=="softmax") usize(1); return makeType("Tensor"); }
            if (member=="sum" || member=="mean" || member=="l2Norm") { need(1,"Tensor."+member); tensor(0); return makeType("f64"); }
            if (member=="dot") { need(2,"Tensor.dot"); tensor(0); tensor(1); return makeType("f64"); }
            if (member=="argmax") { need(2,"Tensor.argmax"); tensor(0); usize(1); return makeType("usize"); }
            if (member=="reshape2" || member=="reshape3" || member=="reshape4") { const auto n=(std::size_t)(member.back()-'0'); need(1+n,"Tensor."+member); tensor(0); for(std::size_t i=0;i<n;++i) usize(1+i); return makeType("Tensor"); }
            if (member=="transpose2") { need(1,"Tensor.transpose2"); tensor(0); return makeType("Tensor"); }
            if (member=="slice") { need(5,"Tensor.slice"); tensor(0); usize(1); usize(2); usize(3); usize(4); return makeType("Tensor"); }
            if (member=="contiguous") { need(1,"Tensor.contiguous"); tensor(0); return makeType("Tensor"); }
            if (member=="fill") { need(2,"Tensor.fill"); tensor(0); f64(1); return makeType("void"); }
            if (member=="conv2d") { need(4,"Tensor.conv2d"); tensor(0); tensor(1); usize(2); usize(3); return makeType("Tensor"); }
            error(expr,"unknown Tensor method '"+member+"'");
        }
        if (ns == "Grad") {
            const auto need = [&](std::size_t n, const std::string& what){ if (expr->args.size()!=n) error(expr, what+" has wrong arity"); };
            if(member=="create"){ need(0,"Grad.create"); return makeType("GradTape"); }
            if(member=="watch"){ need(2,"Grad.watch"); checkExpr(expr->args[0].get(),makeType("GradTape").get()); checkExpr(expr->args[1].get(),makeType("Tensor").get()); return makeType("void"); }
            const auto tape=[&](std::size_t i){checkExpr(expr->args[i].get(),makeType("GradTape").get());};
            const auto ten=[&](std::size_t i){checkExpr(expr->args[i].get(),makeType("Tensor").get());};
            if(member=="add"||member=="mul"||member=="matmul"){need(3,"Grad."+member);tape(0);ten(1);ten(2);return makeType("Tensor");}
            if(member=="relu"||member=="tanh"||member=="sum"){need(2,"Grad."+member);tape(0);ten(1);return makeType("Tensor");}
            if(member=="scale"){need(3,"Grad.scale");tape(0);ten(1);checkExpr(expr->args[2].get(),makeType("f64").get());return makeType("Tensor");}
            if(member=="backward"){need(2,"Grad.backward");tape(0);ten(1);return makeType("void");}
            if(member=="grad"){need(2,"Grad.grad");tape(0);ten(1);return makeType("Tensor");}
            if(member=="free"){need(1,"Grad.free");tape(0);return makeType("void");}
            error(expr,"unknown Grad method '"+member+"'");
        }
        if (ns == "Accel") {
            if(member=="cudaAvailable"||member=="rocmAvailable"||member=="metalAvailable"||member=="blasAvailable"){ if(!expr->args.empty()) error(expr,"Accel."+member+" takes no arguments"); return makeType("bool"); }
            if(member=="backend"){ if(!expr->args.empty()) error(expr,"Accel.backend takes no arguments"); return makeType("string"); }
            error(expr,"unknown Accel method '"+member+"'");
        }
        if (ns == "Buffer") {
            const auto need = [&](std::size_t n, const std::string& what){if(expr->args.size()!=n)error(expr,what+" has wrong arity");};
            if (member=="new") { need(1,"Buffer.new"); checkExpr(expr->args[0].get(),makeType("usize").get()); return makeType("Buffer"); }
            if (member=="fromString") { need(1,"Buffer.fromString"); checkExpr(expr->args[0].get(),makeType("string").get()); return makeType("Buffer"); }
            error(expr,"unknown Buffer method '"+member+"'");
        }
        if (ns == "Args") {
            const auto need = [&](std::size_t n, const std::string& what){ if(expr->args.size()!=n) error(expr,what+" has wrong arity"); };
            if (member=="count") { need(0,"Args.count"); return makeType("usize"); }
            if (member=="at") { need(1,"Args.at"); checkExpr(expr->args[0].get(),makeType("usize").get()); return makeType("string"); }
            error(expr,"unknown Args method '"+member+"'");
        }
        if (ns == "Env") {
            const auto need = [&](std::size_t n, const std::string& what){ if(expr->args.size()!=n) error(expr,what+" has wrong arity"); };
            if (member=="get" || member=="has") { need(1,"Env."+member); checkExpr(expr->args[0].get(),makeType("string").get()); return member=="get"?makeType("string"):makeType("bool"); }
            if (member=="set") { need(2,"Env.set"); checkExpr(expr->args[0].get(),makeType("string").get()); checkExpr(expr->args[1].get(),makeType("string").get()); return makeType("bool"); }
            if (member=="unset") { need(1,"Env.unset"); checkExpr(expr->args[0].get(),makeType("string").get()); return makeType("bool"); }
            error(expr,"unknown Env method '"+member+"'");
        }
        if (ns == "FS") {
            const auto need = [&](std::size_t n, const std::string& what){ if(expr->args.size()!=n) error(expr,what+" has wrong arity"); };
            const auto str = [&](std::size_t i){ checkExpr(expr->args[i].get(),makeType("string").get()); };
            if (member=="exists"||member=="isFile"||member=="isDir") { need(1,"FS."+member); str(0); return makeType("bool"); }
            if (member=="fileSize") { need(1,"FS.fileSize"); str(0); return makeType("i64"); }
            if (member=="read") { need(1,"FS.read"); str(0); return makeType("Buffer"); }
            if (member=="write"||member=="append") { need(2,"FS."+member); str(0); checkExpr(expr->args[1].get(),makeType("string").get()); return makeType("bool"); }
            if (member=="writeBuffer") { need(2,"FS.writeBuffer"); str(0); checkExpr(expr->args[1].get(),makeType("Buffer").get()); return makeType("bool"); }
            if (member=="remove"||member=="mkdir"||member=="rmdir") { need(1,"FS."+member); str(0); return makeType("bool"); }
            if (member=="rename"||member=="copy") { need(2,"FS."+member); str(0); str(1); return makeType("bool"); }
            if (member=="cwd") { need(0,"FS.cwd"); return makeType("string"); }
            if (member=="chdir") { need(1,"FS.chdir"); str(0); return makeType("bool"); }
            if (member=="list") { need(1,"FS.list"); str(0); return makeType("Buffer"); }
            error(expr,"unknown FS method '"+member+"'");
        }
        if (ns == "Path") {
            const auto need = [&](std::size_t n, const std::string& what){ if(expr->args.size()!=n) error(expr,what+" has wrong arity"); };
            const auto str = [&](std::size_t i){ checkExpr(expr->args[i].get(),makeType("string").get()); };
            if (member=="join") { need(2,"Path.join"); str(0); str(1); return makeType("string"); }
            if (member=="basename"||member=="dirname"||member=="extension"||member=="stem"||member=="normalize") { need(1,"Path."+member); str(0); return makeType("string"); }
            if (member=="isAbsolute") { need(1,"Path.isAbsolute"); str(0); return makeType("bool"); }
            if (member=="absolute") { need(1,"Path.absolute"); str(0); return makeType("string"); }
            error(expr,"unknown Path method '"+member+"'");
        }
        if (ns == "String") {
            if (member=="parseInt") { if (expr->args.size()!=1) error(expr,"String.parseInt takes one string"); checkExpr(expr->args[0].get(),makeType("string").get()); return makeType("i64"); }
            if (member=="parseFloat") { if (expr->args.size()!=1) error(expr,"String.parseFloat takes one string"); checkExpr(expr->args[0].get(),makeType("string").get()); return makeType("f64"); }
            error(expr,"unknown String method '"+member+"'");
        }
        if (ns == "Regex") {
            const auto need = [&](std::size_t n, const std::string& what){ if(expr->args.size()!=n) error(expr,what+" has wrong arity"); };
            if (member=="compile") { need(1,"Regex.compile"); checkExpr(expr->args[0].get(),makeType("string").get()); return makeType("Regex"); }
            if (member=="isMatch") { need(2,"Regex.isMatch"); checkExpr(expr->args[0].get(),makeType("Regex").get()); checkExpr(expr->args[1].get(),makeType("string").get()); return makeType("bool"); }
            if (member=="find") { need(2,"Regex.find"); checkExpr(expr->args[0].get(),makeType("Regex").get()); checkExpr(expr->args[1].get(),makeType("string").get()); return makeType("i64"); }
            if (member=="free") { need(1,"Regex.free"); checkExpr(expr->args[0].get(),makeType("Regex").get()); return makeType("void"); }
            error(expr,"unknown Regex method '"+member+"'");
        }
        if (ns == "Shell") {
            const auto need = [&](std::size_t n, const std::string& what){ if(expr->args.size()!=n) error(expr,what+" has wrong arity"); };
            if (member=="run") { need(1,"Shell.run"); checkExpr(expr->args[0].get(),makeType("string").get()); return makeType("i32"); }
            if (member=="output") { need(1,"Shell.output"); checkExpr(expr->args[0].get(),makeType("string").get()); return makeType("Buffer"); }
            if (member=="which") { need(1,"Shell.which"); checkExpr(expr->args[0].get(),makeType("string").get()); return makeType("string"); }
            error(expr,"unknown Shell method '"+member+"'");
        }
        if (ns == "Process") {
            const auto need = [&](std::size_t n, const std::string& what){if(expr->args.size()!=n)error(expr,what+" has wrong arity");};
            if (member=="run") { need(1,"Process.run"); checkExpr(expr->args[0].get(),makeType("string").get()); return makeType("i32"); }
            if (member=="spawn") { need(1,"Process.spawn"); checkExpr(expr->args[0].get(),makeType("string").get()); return makeType("Process"); }
            if (member=="wait") { need(1,"Process.wait"); checkExpr(expr->args[0].get(),makeType("Process").get()); return makeType("i32"); }
            if (member=="argCount") { need(0,"Process.argCount"); return makeType("usize"); }
            if (member=="arg") { need(1,"Process.arg"); checkExpr(expr->args[0].get(),makeType("usize").get()); return makeType("string"); }
            if (member=="output") { need(1,"Process.output"); checkExpr(expr->args[0].get(),makeType("string").get()); return makeType("Buffer"); }
            if (member=="pid") { need(1,"Process.pid"); checkExpr(expr->args[0].get(),makeType("Process").get()); return makeType("u64"); }
            if (member=="terminate") { need(1,"Process.terminate"); checkExpr(expr->args[0].get(),makeType("Process").get()); return makeType("bool"); }
            if (member=="setEnv") { need(2,"Process.setEnv"); checkExpr(expr->args[0].get(),makeType("string").get()); checkExpr(expr->args[1].get(),makeType("string").get()); return makeType("bool"); }
            error(expr,"unknown Process method '"+member+"'");
        }
        if (ns == "Http") {
            if (member=="get") { if(expr->args.size()!=2) error(expr,"Http.get requires url and timeout_ms"); checkExpr(expr->args[0].get(),makeType("string").get()); checkExpr(expr->args[1].get(),makeType("i32").get()); return makeType("Buffer"); }
            if (member=="post") { if(expr->args.size()!=3) error(expr,"Http.post requires url, body, and timeout_ms"); checkExpr(expr->args[0].get(),makeType("string").get()); checkExpr(expr->args[1].get(),makeType("string").get()); checkExpr(expr->args[2].get(),makeType("i32").get()); return makeType("Buffer"); }
            if (member=="status") { if(!expr->args.empty()) error(expr,"Http.status takes no arguments"); return makeType("i32"); }
            error(expr,"unknown Http method '"+member+"'");
        }
        if (ns == "Json") {
            if (member=="validate") { if(expr->args.size()!=1) error(expr,"Json.validate takes one string"); checkExpr(expr->args[0].get(),makeType("string").get()); return makeType("bool"); }
            if (member=="quote") { if(expr->args.size()!=1) error(expr,"Json.quote takes one string"); checkExpr(expr->args[0].get(),makeType("string").get()); return makeType("Buffer"); }
            if (member=="int") { if(expr->args.size()!=1) error(expr,"Json.int takes one i64"); checkExpr(expr->args[0].get(),makeType("i64").get()); return makeType("Buffer"); }
            if (member=="float") { if(expr->args.size()!=1) error(expr,"Json.float takes one f64"); checkExpr(expr->args[0].get(),makeType("f64").get()); return makeType("Buffer"); }
            if (member=="bool") { if(expr->args.size()!=1) error(expr,"Json.bool takes one bool"); checkExpr(expr->args[0].get(),makeType("bool").get()); return makeType("Buffer"); }
            if (member=="nullValue") { if(!expr->args.empty()) error(expr,"Json.nullValue takes no arguments"); return makeType("Buffer"); }
            error(expr,"unknown Json method '"+member+"'");
        }
    }

    if (expr->callee->kind == ExprKind::FieldAccess && expr->callee->target &&
        expr->callee->target->kind == ExprKind::Identifier &&
        expr->callee->target->strValue == "Atomic") {
        const auto member = expr->callee->field;
        if (member == "new") {
            if (expr->args.size() != 1 || !expected || !isAtomic(expected)) {
                // Contextual Atomic.new(T) needs the surrounding atomic type.
                error(expr, "Atomic.new() requires an Atomic[T] expected type");
            }
            checkExpr(expr->args[0].get(), expected->generics[0].get());
            return cloneType(expected);
        }
    }

    if (expr->callee->kind == ExprKind::Identifier) {
        if (auto* calleeSym = symbols.resolve(expr->callee->strValue); calleeSym && calleeSym->type && calleeSym->type->name == "fn") {
            const auto* fnType = calleeSym->type;
            if (fnType->isUnsafeFunction && unsafeDepth <= 0) {
                error(expr, "calling an unsafe function pointer requires an unsafe block");
            }
            const std::size_t arity = fnType->generics.size() - 1;
            if (expr->args.size() != arity) error(expr, "function pointer expects " + std::to_string(arity) + " arguments, got " + std::to_string(expr->args.size()));
            for (std::size_t i = 0; i < arity; ++i) checkExpr(expr->args[i].get(), fnType->generics[i].get());
            return cloneType(fnType->generics.back().get());
        }
        auto it = functions.find(expr->callee->strValue);
        if (it == functions.end()) error(expr, "unknown function '" + expr->callee->strValue + "'");
        auto* fn = it->second;
        if (fn->isUnsafe && unsafeDepth <= 0) {
            error(expr, "calling unsafe function '" + fn->name + "' requires an unsafe block");
        }
        if (expr->args.size() != fn->params.size()) {
            error(expr, "function '" + fn->name + "' expects " + std::to_string(fn->params.size()) +
                          " arguments, got " + std::to_string(expr->args.size()));
        }
        for (std::size_t i = 0; i < expr->args.size(); ++i) {
            auto argumentType = checkExpr(expr->args[i].get(), fn->params[i].type.get());
            if (!fn->params[i].type->isReference && !isView(fn->params[i].type.get())) {
                rejectBorrowedOwnerProjection(expr->args[i].get(), argumentType.get(), "owning function argument");
            }
            if (!fn->params[i].type->isReference && !isView(fn->params[i].type.get()) &&
                hasShortLivedNestedOrigin(argumentType.get())) {
                // The callee receives ownership, so any nested view/reference or
                // arena-backed storage would otherwise be able to outlive its source.
                // Until function signatures carry a precise provenance contract, reject
                // this conservatively with the origin named in the diagnostic.
                error(expr, "cannot pass a value containing " + shortLivedOriginDescription(argumentType.get()) +
                            " to an owning parameter");
            }
            // Passing an owned non-copy value transfers ownership into the call.
            if (expr->args[i]->kind == ExprKind::Identifier && !fn->params[i].type->isReference && !isView(fn->params[i].type.get())) {
                if (auto* src = symbols.resolve(expr->args[i]->strValue); src && !isCopyType(src->type)) {
                    rejectMoveWhileBorrowed(src, expr->args[i].get(), "owning function argument");
                    src->memory.markMoved();
                }
            }
        }
        auto result = cloneType(fn->returnType.get());
        const auto returnBorrow = functionReturnBorrowParam.find(fn->name);
        if (returnBorrow != functionReturnBorrowParam.end() &&
            (result->isReference || isView(result.get()))) {
            const auto index = returnBorrow->second;
            if (index >= expr->args.size()) error(expr, "internal error: invalid borrow-return parameter index");
            auto argumentType = inferExprType(expr->args[index].get());
            result->origin = argumentType->origin;
            if (!result->origin.valid()) {
                const auto ownerName = borrowRoot(expr->args[index].get());
                if (!ownerName.empty()) {
                    if (auto* owner = symbols.resolve(ownerName)) result->origin = inferOrigin(owner, ownerName);
                }
            }
            if (!result->origin.valid()) {
                error(expr, "cannot determine the storage origin of a returned reference/view");
            }
        }
        if (expected && !compatible(result.get(), expected)) error(expr, "expected " + typeToString(expected) + ", got " + typeToString(result.get()));
        return result;
    }

    if (expr->callee->kind == ExprKind::FieldAccess) {
        auto* field = expr->callee.get();
        auto target = inferExprType(field->target.get());
        if (target->name == "Buffer") {
            const auto& member = field->field;
            if (member == "len") { if (!expr->args.empty()) error(expr, "Buffer.len() takes no arguments"); return makeType("usize"); }
            if (member == "data") { if (!expr->args.empty()) error(expr, "Buffer.data() takes no arguments"); auto p=makeType("u8"); p->isRawPointer=true; p->isMutable=true; return p; }
            if (member == "cstr") { if (!expr->args.empty()) error(expr, "Buffer.cstr() takes no arguments"); return makeType("string"); }
            if (member == "free") { if (!expr->args.empty()) error(expr, "Buffer.free() takes no arguments"); return makeType("void"); }
            if (member == "appendString") { if (expr->args.size()!=1) error(expr, "Buffer.appendString takes one string"); checkExpr(expr->args[0].get(), makeType("string").get()); return makeType("bool"); }
            if (member == "appendBuffer") { if (expr->args.size()!=1) error(expr, "Buffer.appendBuffer takes one Buffer"); checkExpr(expr->args[0].get(), makeType("Buffer").get()); return makeType("bool"); }
            error(expr, "unknown Buffer method '" + member + "'");
        }
        if (target->isRawPointer) {
            if (unsafeDepth <= 0) error(expr, "raw pointer arithmetic requires an unsafe block");
            if (field->field != "add" && field->field != "sub" && field->field != "offset") {
                error(expr, "unknown raw pointer method '" + field->field + "'");
            }
            if (expr->args.size() != 1) error(expr, "raw pointer arithmetic takes exactly one offset");
            checkExpr(expr->args[0].get(), makeType("isize").get());
            return cloneType(target.get());
        }
        if (isAtomic(target.get())) {
            const std::string member = field->field;
            const auto checkOrder = [&](std::size_t index, const char* context) {
                if (index >= expr->args.size()) return std::string("seq_cst");
                if (expr->args[index]->kind != ExprKind::StringLit) error(expr, std::string(context) + " memory order must be a string literal");
                const auto order = expr->args[index]->strValue;
                if (order != "relaxed" && order != "acquire" && order != "release" && order != "acq_rel" && order != "seq_cst") error(expr, "invalid atomic memory order '" + order + "'");
                return order;
            };
            if (member == "load") {
                if (expr->args.size() > 1) error(expr, "Atomic.load() takes an optional memory-order string");
                const auto order = checkOrder(0, "Atomic.load");
                if (order == "release" || order == "acq_rel") error(expr, "Atomic.load cannot use release ordering");
                return cloneType(target->generics[0].get());
            }
            if (member == "store") { if (expr->args.size() < 1 || expr->args.size() > 2) error(expr, "Atomic.store() takes a value and optional memory-order string"); checkExpr(expr->args[0].get(), target->generics[0].get()); const auto order=checkOrder(1,"Atomic.store"); if (order=="acquire" || order=="acq_rel") error(expr,"Atomic.store cannot use acquire-only ordering"); return makeType("void"); }
            if (member == "fetchAdd" || member == "fetchSub") { if (expr->args.size() < 1 || expr->args.size() > 2) error(expr, "atomic fetch operation takes a value and optional memory-order string"); checkExpr(expr->args[0].get(), target->generics[0].get()); checkOrder(1,"atomic fetch"); return cloneType(target->generics[0].get()); }
            if (member == "compareExchange") { if (expr->args.size() < 2 || expr->args.size() > 3) error(expr, "Atomic.compareExchange() takes expected, desired, and optional memory-order string"); checkExpr(expr->args[0].get(), target->generics[0].get()); checkExpr(expr->args[1].get(), target->generics[0].get()); checkOrder(2,"Atomic.compareExchange"); return makeType("bool"); }
            error(expr, "unknown Atomic method '" + member + "'");
        }
        if (target->name == "Thread" && !target->isArray && !target->isReference) {
            if (field->field == "join" || field->field == "detach") {
                if (!expr->args.empty()) error(expr, "Thread handle method takes no arguments");
                return makeType("void");
            }
        }
        if (target->name == "GradTape" && !target->isArray && !target->isReference) {
            if (field->field == "free") { if (!expr->args.empty()) error(expr, "GradTape.free() takes no arguments"); return makeType("void"); }
            error(expr, "unknown GradTape method '" + field->field + "'");
        }
        if (target->name == "Tensor" && !target->isArray && !target->isReference) {
            const std::string member = field->field;
            const auto need = [&](std::size_t n, const std::string& what){ if (expr->args.size() != n) error(expr, what + " has wrong arity"); };
            const auto usize = [&](std::size_t i){ checkExpr(expr->args[i].get(), makeType("usize").get()); };
            const auto f64 = [&](std::size_t i){ checkExpr(expr->args[i].get(), makeType("f64").get()); };
            const auto tensor = [&](std::size_t i){ checkExpr(expr->args[i].get(), makeType("Tensor").get()); };
            if (member == "clone" || member == "contiguous") { need(0, "Tensor." + member); return makeType("Tensor"); }
            if (member == "free") { need(0, "Tensor.free"); return makeType("void"); }
            if (member == "rank" || member == "len") { need(0, "Tensor." + member); return makeType("usize"); }
            if (member == "dim" || member == "stride") { need(1, "Tensor." + member); usize(0); return makeType("usize"); }
            if (member == "dtype" || member == "isContiguous") { need(0, "Tensor." + member); return makeType(member == "dtype" ? "u32" : "bool"); }
            if (member == "dataF32" || member == "dataF64") {
                need(0, "Tensor." + member);
                if (unsafeDepth <= 0) error(expr, "Tensor." + member + " requires an unsafe block");
                auto t = makeType(member == "dataF64" ? "f64" : "f32"); t->isRawPointer = true; t->isMutable = true; return t;
            }
            if (member == "get1" || member == "get2" || member == "get3") { const auto n=(std::size_t)(member.back()-'0'); need(n,"Tensor."+member); for(std::size_t i=0;i<n;++i) usize(i); return makeType("f64"); }
            if (member == "set1" || member == "set2" || member == "set3") { const auto n=(std::size_t)(member.back()-'0'); need(n+1,"Tensor."+member); for(std::size_t i=0;i<n;++i) usize(i); f64(n); return makeType("void"); }
            if (member == "add" || member == "sub" || member == "mul" || member == "div" || member == "matmul") { need(1,"Tensor."+member); tensor(0); return makeType("Tensor"); }
            if (member == "scale") { need(1,"Tensor.scale"); f64(0); return makeType("Tensor"); }
            if (member == "relu" || member == "sigmoid" || member == "tanh") { need(0,"Tensor."+member); return makeType("Tensor"); }
            if (member == "softmax") { need(1,"Tensor.softmax"); usize(0); return makeType("Tensor"); }
            if (member == "sum" || member == "mean" || member == "l2Norm") { need(0,"Tensor."+member); return makeType("f64"); }
            if (member == "dot") { need(1,"Tensor.dot"); tensor(0); return makeType("f64"); }
            if (member == "argmax") { need(1,"Tensor.argmax"); usize(0); return makeType("usize"); }
            if (member == "reshape2" || member == "reshape3" || member == "reshape4") { const auto n=(std::size_t)(member.back()-'0'); need(n,"Tensor."+member); for(std::size_t i=0;i<n;++i) usize(i); return makeType("Tensor"); }
            if (member == "transpose2") { need(0,"Tensor.transpose2"); return makeType("Tensor"); }
            if (member == "slice") { need(4,"Tensor.slice"); usize(0); usize(1); usize(2); usize(3); return makeType("Tensor"); }
            if (member == "fill") { need(1,"Tensor.fill"); f64(0); return makeType("void"); }
            if (member == "conv2d") { need(3,"Tensor.conv2d"); tensor(0); usize(1); usize(2); return makeType("Tensor"); }
            error(expr,"unknown Tensor method '" + member + "'");
        }
        if (isVector(target.get())) {
            if (field->field == "extractU64") {
                if (expr->args.size()!=1) error(expr, "vector extractU64() takes one lane argument");
                checkExpr(expr->args[0].get(), makeType("u32").get());
                return makeType("u64");
            }
            if (field->field == "add") {
                if (expr->args.size()!=1) error(expr, "vector add() takes one vector argument");
                checkExpr(expr->args[0].get(), target.get());
                return cloneType(target.get());
            }
            if (field->field == "and" || field->field == "or" || field->field == "xor") {
                if (expr->args.size()!=1) error(expr, "vector bit operation takes one vector argument");
                checkExpr(expr->args[0].get(), target.get());
                return cloneType(target.get());
            }
        }
        const bool targetWasMutableReference = target->isReference && target->isMutable;
        if (target->isReference) target->isReference = false;

        if (isView(target.get()) && field->field == "len") {
            if (!expr->args.empty()) error(expr, "len() takes no arguments");
            return makeType("usize");
        }
        if (target->name == "string" && !target->isArray) {
            const std::string member = field->field;
            if (member == "len") { if (!expr->args.empty()) error(expr,"len() takes no arguments"); return makeType("usize"); }
            if (member == "contains" || member == "startsWith" || member == "endsWith") { if (expr->args.size()!=1) error(expr,member+"() takes one string"); checkExpr(expr->args[0].get(),makeType("string").get()); return makeType("bool"); }
            if (member == "find") { if (expr->args.size()!=1) error(expr,"find() takes one string"); checkExpr(expr->args[0].get(),makeType("string").get()); return makeType("i64"); }
            if (member == "equalsIgnoreCase") { if (expr->args.size()!=1) error(expr,"equalsIgnoreCase() takes one string"); checkExpr(expr->args[0].get(),makeType("string").get()); return makeType("bool"); }
        }
        if (target->isArray) {
            if (field->field == "isEmpty") {
                if (!expr->args.empty()) error(expr, "isEmpty() takes no arguments");
                return makeType("bool");
            }
            if (field->field == "len") {
                if (!expr->args.empty()) error(expr, "len() takes no arguments");
                return makeType("usize");
            }
            if (field->field == "push") {
                if (target->fixedArraySize) error(expr, "fixed arrays have no push(); use an index assignment");
                if (expr->args.size() != 1) error(expr, "push() takes exactly one argument");

                // Mutating a dynamic array requires exclusive access. A direct local
                // is mutable unless it is const/borrowed; a &mut []T target is the
                // explicit escape hatch for helper functions.
                if (field->target->kind == ExprKind::Identifier) {
                    if (!targetWasMutableReference) {
                        if (auto* owner = symbols.resolve(field->target->strValue)) {
                            if (owner->isConst) error(expr, "cannot push into const array");
                            if (owner->memory.isMoved()) error(expr, "cannot push into moved array");
                            if (owner->memory.hasAnyBorrow()) error(expr, "cannot push while the array has an active borrow/view");
                        }
                    }
                } else if (targetWasMutableReference) {
                    // Allowed through an explicit &mut reference.
                } else {
                    error(expr, "push() requires an owned array or &mut []T");
                }

                auto elem = cloneType(target.get());
                elem->isArray = false;
                elem->fixedArraySize.reset();
                elem->origin = {};
                elem->isReference = false;
                elem->isMutable = false;
                checkExpr(expr->args[0].get(), elem.get());
                rejectBorrowedOwnerProjection(expr->args[0].get(), elem.get(), "push()");
                // push() stores the value inside the owning array, so an owned
                // identifier passed as the element is moved into the collection.
                if (expr->args[0]->kind == ExprKind::Identifier && !isCopyType(elem.get())) {
                    if (auto* source = symbols.resolve(expr->args[0]->strValue)) {
                        rejectMoveWhileBorrowed(source, expr->args[0].get(), "push()");
                        source->memory.markMoved();
                    }
                }
                if (field->target->kind == ExprKind::Identifier) {
                    if (auto* destination = symbols.resolve(field->target->strValue)) {
                        mergeNestedOrigins(destination->type, expr->args[0]->checkedType.get());
                    }
                }
                return makeType("void");
            }
        }
        error(expr, "unknown member call '" + field->field + "'");
    }

    error(expr, "only direct functions and built-in collection methods can be called");
}

std::unique_ptr<TypeNode> TypeChecker::checkFieldAccess(const Expr* expr) {
    if (expr->target->kind == ExprKind::Identifier) {
        const std::string& targetName = expr->target->strValue;
        if (!symbols.resolve(targetName)) {
            if (enums.count(targetName)) {
                for (const auto& v : enums.at(targetName)->variants) {
                    if (v.name == expr->field) return makeType(targetName);
                }
                error(expr, "enum '" + targetName + "' has no variant '" + expr->field + "'");
            }
            // Type-qualified constants are intentionally allowed for nominal types.
            // This keeps older error-code style APIs such as Error.NotFound valid,
            // while real enums receive full variant validation above.
            if (structs.count(targetName)) return makeType(targetName);
        }
    }

    auto target = inferExprType(expr->target.get());
    if (target->isReference) target->isReference = false;
    if (target->isArray || isView(target.get())) error(expr, "collection values expose methods through calls, not fields");

    auto it = structs.find(target->name);
    if (it == structs.end()) error(expr, "type '" + typeToString(target.get()) + "' has no field '" + expr->field + "'");
    for (const auto& f : it->second->fields) {
        if (f.name == expr->field) return cloneType(f.type.get());
    }
    error(expr, "struct '" + target->name + "' has no field '" + expr->field + "'");
}

std::unique_ptr<TypeNode> TypeChecker::checkStructLiteral(const Expr* expr) {
    auto it = structs.find(expr->structName);
    if (it == structs.end()) error(expr, "unknown struct '" + expr->structName + "'");

    std::map<std::string, const Expr*> supplied;
    for (const auto& f : expr->fields) {
        if (!supplied.emplace(f.first, f.second.get()).second) error(expr, "field '" + f.first + "' is specified more than once");
    }
    auto result = makeType(expr->structName);
    for (const auto& f : it->second->fields) {
        auto fit = supplied.find(f.name);
        if (fit == supplied.end()) error(expr, "missing field '" + f.name + "' in struct literal");
        auto fieldType = checkExpr(fit->second, f.type.get());
        mergeNestedOrigins(result.get(), fieldType.get());
        rejectBorrowedOwnerProjection(fit->second, f.type.get(), "struct literal");
        // Struct construction transfers ownership of owned field values.
        if (fit->second->kind == ExprKind::Identifier && !isCopyType(f.type.get())) {
            if (auto* source = symbols.resolve(fit->second->strValue)) {
                rejectMoveWhileBorrowed(source, fit->second, "struct literal");
                source->memory.markMoved();
            }
        }
    }
    if (supplied.size() != it->second->fields.size()) error(expr, "unknown field in struct literal");

    return result;
}

std::unique_ptr<TypeNode> TypeChecker::checkIsMatch(const Expr* expr) {
    auto result = inferExprType(expr->left.get());
    if (!isResult(result.get())) error(expr, "'is Ok/Err' requires a Result value");
    if (expr->matchKind != "Ok" && expr->matchKind != "Err") error(expr, "unknown Result match kind");
    return makeType("bool");
}

std::unique_ptr<TypeNode> TypeChecker::checkResultLiteral(const Expr* expr, const TypeNode* expected) {
    if (!expected || !isResult(expected)) error(expr, "Ok(...) and Err(...) require a Result context");
    const bool ok = expr->kind == ExprKind::OkLit;
    const auto* payloadType = expected->generics[ok ? 0 : 1].get();
    auto payloadChecked = checkExpr(expr->value.get(), payloadType);
    auto resultType = cloneType(expected);
    mergeNestedOrigins(resultType.get(), payloadChecked.get());
    rejectBorrowedOwnerProjection(expr->value.get(), payloadType, ok ? "Ok(...)" : "Err(...)");

    // Constructing a Result consumes an owned non-copy payload just like a normal
    // function call or assignment. Without this, `return Ok(moves)` would leave the
    // source local alive and the backend could later clean it twice.
    if (expr->value->kind == ExprKind::Identifier && !isCopyType(payloadType)) {
        if (auto* source = symbols.resolve(expr->value->strValue)) {
            rejectMoveWhileBorrowed(source, expr->value.get(), "Result construction");
            source->memory.markMoved();
        }
    }
    return resultType;
}

std::unique_ptr<TypeNode> TypeChecker::checkCast(const Expr* expr) {
    validateType(expr->castType.get(), expr->line);
    auto source = inferExprType(expr->value.get());
    const auto* dest = expr->castType.get();
    if ((source->isRawPointer || dest->isRawPointer) && unsafeDepth <= 0) {
        error(expr, "raw-pointer casts require an unsafe block");
    }
    const bool sourceFunctionPointer = source->name == "fn" && !source->isArray && !source->isReference && !source->isRawPointer;
    const bool destFunctionPointer = dest->name == "fn" && !dest->isArray && !dest->isReference && !dest->isRawPointer;
    if ((source->isRawPointer && dest->isRawPointer) ||
        (source->isRawPointer && isInteger(dest)) ||
        (isInteger(source.get()) && dest->isRawPointer) ||
        (sourceFunctionPointer && (dest->isRawPointer || isInteger(dest))) ||
        (destFunctionPointer && (source->isRawPointer || isInteger(source.get()))) ||
        (sourceFunctionPointer && destFunctionPointer) ||
        (source->isRawPointer && dest->name == "string" && !dest->isArray && !dest->isOptional && !dest->isReference) ||
        (source->name == "string" && !source->isArray && !source->isOptional && !source->isReference && dest->isRawPointer) ||
        (isNumeric(source.get()) && isNumeric(dest)) ||
        (isBoolean(source.get()) && isInteger(dest)) ||
        (isInteger(source.get()) && isBoolean(dest)) ||
        (isBoolean(source.get()) && isBoolean(dest))) {
        return cloneType(dest);
    }
    error(expr, "cannot cast " + typeToString(source.get()) + " to " + typeToString(dest));
}

std::unique_ptr<TypeNode> TypeChecker::checkLValue(const Expr* expr) {
    if (!expr) throw std::runtime_error("invalid assignment target");
    switch (expr->kind) {
        case ExprKind::Identifier: {
            auto* sym = symbols.resolve(expr->strValue);
            if (!sym) error(expr, "unknown assignment target '" + expr->strValue + "'");
            if (sym->isConst) error(expr, "cannot assign to const '" + expr->strValue + "'");
            if (sym->memory.isMoved()) error(expr, "cannot assign to moved value '" + expr->strValue + "'");
            if (sym->memory.hasAnyBorrow()) error(expr, "cannot modify '" + expr->strValue + "' while it is borrowed");

            // An identifier bound to &mut T denotes the borrowed T for lvalue
            // assignment. The reference binding itself is not rebound; the store
            // must land in the pointee. This also keeps `v = value` consistent with
            // field/index mutation through the same exclusive reference.
            auto target = cloneType(sym->type);
            if (target->isReference) {
                if (!target->isMutable) error(expr, "cannot modify through a read-only reference");
                target->isReference = false;
                target->isMutable = false;
                target->origin = {};
                target->nestedOrigins.clear();
            }
            return target;
        }
        case ExprKind::FieldAccess: {
            auto target = inferExprType(expr->target.get());
            if (target->isReference) {
                if (!target->isMutable) error(expr, "cannot modify through a read-only reference");
            } else {
                const auto root = borrowRoot(expr->target.get());
                if (!root.empty()) {
                    if (auto* owner = symbols.resolve(root); owner && (owner->memory.hasAnyBorrow())) {
                        error(expr, "cannot modify an owner while it is borrowed");
                    }
                }
            }
            auto fieldType = checkFieldAccess(expr);
            const_cast<Expr*>(expr)->checkedType = cloneType(fieldType.get());
            return fieldType;
        }
        case ExprKind::UnaryOp: {
            if (expr->op != "*") error(expr, "expression is not assignable");
            if (unsafeDepth <= 0) error(expr, "raw-pointer dereference requires an unsafe block");
            auto p = inferExprType(expr->value.get());
            if (!p->isRawPointer || !p->isMutable) error(expr, "cannot assign through a const raw pointer");
            auto pointee = cloneType(p.get());
            pointee->isRawPointer = false;
            pointee->isMutable = false;
            return pointee;
        }
        case ExprKind::Index: {
            auto target = inferExprType(expr->target.get());
            if (target->isRawPointer) {
                if (unsafeDepth <= 0) error(expr, "raw pointer indexing requires an unsafe block");
                if (!target->isMutable) error(expr, "cannot modify through a const raw pointer");
                if (target->name == "void") error(expr, "cannot index a void raw pointer");
                if (expr->args.size() != 1) error(expr, "raw pointer indexing requires one index");
                auto index = inferExprType(expr->args[0].get());
                if (!isInteger(index.get())) error(expr, "raw pointer index must be an integer");
                auto pointee = cloneType(target.get());
                pointee->isRawPointer = false; pointee->isMutable = false;
                return pointee;
            }
            if (target->name == "View") error(expr, "cannot modify through a read-only View; use EditView");
            if (target->isReference) {
                if (!target->isMutable) error(expr, "cannot modify through a read-only reference");
            } else if (target->name != "EditView") {
                const auto root = borrowRoot(expr->target.get());
                if (!root.empty()) {
                    if (auto* owner = symbols.resolve(root); owner && (owner->memory.hasAnyBorrow())) {
                        error(expr, "cannot modify an owner while it is borrowed");
                    }
                }
            }
            return checkExpr(expr, nullptr);
        }
        default:
            error(expr, "expression is not assignable");
    }
}

void TypeChecker::checkStmt(const Stmt* stmt) {
    if (!stmt) return;

    switch (stmt->kind) {
        case StmtKind::UnsafeBlock:
            ++unsafeDepth;
            checkBlock(stmt->body);
            --unsafeDepth;
            break;

        case StmtKind::ExprStmt:
            inferExprType(stmt->expr.get());
            break;

        case StmtKind::Assign:
        case StmtKind::ConstAssign: {
            const Expr* targetExpr = stmt->expr ? stmt->expr.get() : nullptr;
            Symbol* existing = nullptr;
            if (!targetExpr && !stmt->assignTarget.empty()) {
                // Plain assignment updates the nearest visible binding. A new local is
                // created only when no visible binding exists. This avoids accidental
                // shadowing inside if/while/for scopes and matches ordinary language intuition.
                existing = stmt->declaredType ? symbols.currentScope()->resolveLocal(stmt->assignTarget)
                                              : symbols.resolve(stmt->assignTarget);
            }

            std::unique_ptr<TypeNode> targetType;
            if (targetExpr) targetType = checkLValue(targetExpr);
            else if (existing && existing->isConst) {
                throw std::runtime_error("Assignment at line " + std::to_string(stmt->line) +
                                         " modifies const '" + stmt->assignTarget + "'");
            } else if (existing && existing->type && existing->type->isReference) {
                // A plain identifier assignment to &mut T mutates the pointee; it
                // never rebinds the reference itself. Reuse the same transparent
                // lvalue rule as checkLValue(identifier).
                targetType = cloneType(existing->type);
                if (!targetType->isMutable) {
                    throw std::runtime_error("Assignment at line " + std::to_string(stmt->line) +
                                             " modifies through a read-only reference '" + stmt->assignTarget + "'");
                }
                targetType->isReference = false;
                targetType->isMutable = false;
                targetType->origin = {};
                targetType->nestedOrigins.clear();
            }

            std::unique_ptr<TypeNode> rhs;
            if (stmt->declaredType) {
                validateType(stmt->declaredType.get(), stmt->line);
                rhs = checkExpr(stmt->assignValue.get(), stmt->declaredType.get());
            } else if (targetType) {
                rhs = checkExpr(stmt->assignValue.get(), targetType.get());
            } else if (existing) {
                rhs = checkExpr(stmt->assignValue.get(), existing->type);
            } else {
                rhs = inferExprType(stmt->assignValue.get());
            }

            if (targetExpr) {
                if (!compatible(rhs.get(), targetType.get())) {
                    throw std::runtime_error("Assignment at line " + std::to_string(stmt->line) +
                                             " has incompatible target and value types");
                }
                // Assignment into a field/index transfers ownership of an owned
                // identifier into the destination slot. The destination expression
                // itself is not a local binding, but the source still must be marked moved.
                rejectBorrowedOwnerProjection(stmt->assignValue.get(), rhs.get(), "assignment");
                if (stmt->assignValue->kind == ExprKind::Identifier &&
                    !rhs->isReference && !isView(rhs.get()) && !isCopyType(rhs.get())) {
                    if (auto* source = symbols.resolve(stmt->assignValue->strValue)) {
                        rejectMoveWhileBorrowed(source, stmt->assignValue.get(), "assignment");
                        source->memory.markMoved();
                    }
                }
                const std::string destinationRoot = borrowRoot(stmt->expr.get());
                if (!destinationRoot.empty()) {
                    if (auto* destination = symbols.resolve(destinationRoot)) {
                        mergeNestedOrigins(destination->type, rhs.get());
                    }
                }
                break;
            }

            if (existing && !compatible(rhs.get(), targetType ? targetType.get() : existing->type)) {
                throw std::runtime_error("Assignment at line " + std::to_string(stmt->line) +
                                         " changes type of '" + stmt->assignTarget + "'");
            }

            rejectBorrowedOwnerProjection(stmt->assignValue.get(), rhs.get(), "assignment");
            if (stmt->assignValue->kind == ExprKind::Identifier && !rhs->isReference && !isView(rhs.get())) {
                if (auto* src = symbols.resolve(stmt->assignValue->strValue)) {
                    if (src->name == stmt->assignTarget) {
                        if (!isCopyType(rhs.get())) {
                            throw std::runtime_error("Assignment at line " + std::to_string(stmt->line) +
                                                     " moves a value into itself");
                        }
                            } else if (!isCopyType(rhs.get())) {
                        rejectMoveWhileBorrowed(src, stmt->assignValue.get(), "assignment");
                        src->memory.markMoved();
                    }
                }
            }

            const std::string rhsBorrowOrigin = rhs->origin.binding;
            typePool.push_back(std::move(rhs));
            Symbol sym;
            sym.name = stmt->assignTarget;
            sym.type = typePool.back().get();
            sym.declaredLine = stmt->line;
            sym.isConst = stmt->kind == StmtKind::ConstAssign;

            bool transferredBorrow = false;
            Symbol* sourceSymbol = nullptr;
            if (stmt->assignValue->kind == ExprKind::Identifier) sourceSymbol = symbols.resolve(stmt->assignValue->strValue);
            if (sourceSymbol && !isCopyType(sym.type) && sourceSymbol->memory.borrow) {
                sym.memory.borrow = sourceSymbol->memory.borrow;
                sourceSymbol->memory.borrow.reset();
                sourceSymbol->memory.markMoved();
                transferredBorrow = true;
            }

            // Ownership transfer creates a new local owner lifetime. Arena-backed
            // storage keeps its region origin; every ordinary owning value becomes
            // anchored to the destination binding after the move.
            if (sym.type && !sym.type->isReference && !isView(sym.type) &&
                sym.type->origin.kind != stable::memory::StorageOriginKind::Arena) {
                sym.type->origin = stable::memory::StorageOrigin{
                    stable::memory::StorageOriginKind::Local, stmt->assignTarget};
            }

            if (existing) {
                // Replacing an existing binding ends its previous borrow token first.
                releaseBorrow(existing);
                existing->type = sym.type;
                existing->memory.markLive();
                existing->memory.borrow = sym.memory.borrow;
                existing->isConst = sym.isConst;
            } else {
                symbols.declare(stmt->assignTarget, sym);
            }

            if (!rhsBorrowOrigin.empty() && !transferredBorrow) {
                const auto ownerName = rhsBorrowOrigin;
                if (auto* owner = symbols.resolve(ownerName)) {
                    auto record = registerBorrow(owner, typePool.back().get(), ownerName);
                    if (existing) {
                        existing->memory.borrow = record;
                    } else if (auto* inserted = symbols.currentScope()->resolveLocal(stmt->assignTarget)) {
                        inserted->memory.borrow = record;
                    }
                }
            }
            break;
        }

        case StmtKind::Return:
            if (!currentReturnType) throw std::runtime_error("Return outside function at line " + std::to_string(stmt->line));
            if (!stmt->expr) {
                if (!isVoid(currentReturnType.get())) throw std::runtime_error("Missing return value at line " + std::to_string(stmt->line));
            } else {
                auto returned = checkExpr(stmt->expr.get(), currentReturnType.get());
                rejectBorrowedOwnerProjection(stmt->expr.get(), returned.get(), "return");
                if (stmt->expr->kind == ExprKind::Identifier && !returned->isReference && !isView(returned.get()) && !isCopyType(returned.get())) {
                    if (auto* source = symbols.resolve(stmt->expr->strValue)) {
                        rejectMoveWhileBorrowed(source, stmt->expr.get(), "return");
                        source->memory.markMoved();
                    }
                }
                if (hasShortLivedNestedOrigin(returned.get())) {
                    throw std::runtime_error("cannot return value containing " + shortLivedOriginDescription(returned.get()) +
                                             " from function at line " + std::to_string(stmt->line));
                }
                if (returned->isArray && returned->origin.kind == stable::memory::StorageOriginKind::Arena) {
                    throw std::runtime_error("cannot return an arena-backed array from function at line " +
                                             std::to_string(stmt->line));
                }
                if (isView(returned.get()) && returned->origin.isShortLived()) {
                    throw std::runtime_error("cannot return a View that may reference a local value at line " +
                                             std::to_string(stmt->line));
                }
                if (returned->isReference || isView(returned.get())) {
                    std::string returnOwner = returned->origin.binding;
                    if (returnOwner.empty() && stmt->expr->kind == ExprKind::Identifier) {
                        if (auto* candidate = symbols.resolve(stmt->expr->strValue);
                            candidate && candidate->isParameter && candidate->type &&
                            (candidate->type->isReference || isView(candidate->type))) {
                            returnOwner = candidate->name;
                        }
                    }
                    if (!returnOwner.empty()) {
                        auto* owner = symbols.resolve(returnOwner);
                        const bool externalOwner = owner && owner->isParameter && owner->type &&
                            (owner->type->isReference || isView(owner->type));
                        if (!externalOwner) {
                            throw std::runtime_error("cannot return a borrow tied to local/owned value '" + returnOwner + "' at line " +
                                                     std::to_string(stmt->line));
                        }
                        if (currentFunctionName.size()) recordReturnedBorrow(currentFunctionName, returnOwner, stmt->expr.get());
                    }
                }
            }
            break;

        case StmtKind::Continue:
        case StmtKind::Break:
            if (loopDepth == 0) throw std::runtime_error("loop control outside a loop at line " + std::to_string(stmt->line));
            break;

        case StmtKind::If: {
            auto condition = inferExprType(stmt->expr.get());
            if (!isBoolean(condition.get())) throw std::runtime_error("If condition must be bool at line " + std::to_string(stmt->line));
            symbols.pushScope();
            try {
                checkBlock(stmt->body);
                popCheckedScope();
            } catch (...) {
                popCheckedScope();
                throw;
            }
            if (!stmt->elseBody.empty()) {
                symbols.pushScope();
                try {
                    checkBlock(stmt->elseBody);
                    popCheckedScope();
                } catch (...) {
                    popCheckedScope();
                    throw;
                }
            }
            break;
        }

        case StmtKind::While: {
            auto condition = inferExprType(stmt->expr.get());
            if (!isBoolean(condition.get())) throw std::runtime_error("While condition must be bool at line " + std::to_string(stmt->line));
            symbols.pushScope();
            ++loopDepth;
            try {
                checkBlock(stmt->body);
                --loopDepth;
                popCheckedScope();
            } catch (...) {
                --loopDepth;
                popCheckedScope();
                throw;
            }
            break;
        }

        case StmtKind::For: {
            auto iterable = inferExprType(stmt->iterable.get());
            std::unique_ptr<TypeNode> elem;
            if (iterable->isArray) {
                elem = cloneType(iterable.get());
                elem->isArray = false;
                elem->fixedArraySize.reset();
            } else if (isView(iterable.get())) {
                elem = cloneType(iterable->generics[0].get());
            } else {
                throw std::runtime_error("'for' iterable must be an array or View at line " + std::to_string(stmt->line));
            }
            const std::string iterableBorrowOrigin = !iterable->origin.binding.empty()
                ? iterable->origin.binding : borrowRoot(stmt->iterable.get());
            elem->isReference = true;
            elem->isMutable = false;
            if (!iterableBorrowOrigin.empty()) {
                if (auto* owner = symbols.resolve(iterableBorrowOrigin)) elem->origin = inferOrigin(owner, iterableBorrowOrigin);
            }
            symbols.pushScope();
            try {
                typePool.push_back(std::move(elem));
                Symbol loopSymbol;
                loopSymbol.name = stmt->loopVar;
                loopSymbol.type = typePool.back().get();
                loopSymbol.type->origin = iterableBorrowOrigin.empty() ? stable::memory::StorageOrigin{} :
                    (symbols.resolve(iterableBorrowOrigin) ? inferOrigin(symbols.resolve(iterableBorrowOrigin), iterableBorrowOrigin) : stable::memory::StorageOrigin{});
                loopSymbol.declaredLine = stmt->line;
                loopSymbol.isConst = true;
                symbols.declare(stmt->loopVar, loopSymbol);
                // Iteration exposes element references, so the owner must remain
                // shared-borrowed for the whole loop body. An EditView already owns
                // an exclusive borrow of its origin, so no second borrow is created.
                if (!iterableBorrowOrigin.empty() && !(isView(iterable.get()) && iterable->name == "EditView")) {
                    if (auto* owner = symbols.resolve(iterableBorrowOrigin)) {
                        auto record = registerBorrow(owner, typePool.back().get(), iterableBorrowOrigin);
                        if (auto* inserted = symbols.currentScope()->resolveLocal(stmt->loopVar)) inserted->memory.borrow = record;
                    }
                }
                ++loopDepth;
                checkBlock(stmt->body);
                --loopDepth;
                popCheckedScope();
            } catch (...) {
                if (loopDepth > 0) --loopDepth;
                popCheckedScope();
                throw;
            }
            break;
        }

        case StmtKind::Guard: {
            auto condition = inferExprType(stmt->guardCondition.get());
            if (!isBoolean(condition.get())) throw std::runtime_error("Guard condition must be bool at line " + std::to_string(stmt->line));
            symbols.pushScope();
            try {
                if (stmt->guardCondition->kind == ExprKind::IsMatch) {
                    auto result = inferExprType(stmt->guardCondition->left.get());
                    const std::size_t idx = stmt->guardCondition->matchKind == "Ok" ? 0 : 1;
                    auto bindingType = cloneType(result->generics[idx].get());
                    Symbol binding;
                    binding.name = stmt->guardCondition->bindingName;
                    binding.declaredLine = stmt->line;
                    binding.isConst = true;

                    // Non-copy payloads are borrowed from the matched Result rather than
                    // copied out. This keeps `Result[[]T, E]` and similar values from
                    // creating two owners of the same storage. Copyable payloads remain
                    // ordinary values.
                    if (!isCopyType(bindingType.get())) {
                        const std::string ownerName = borrowRoot(stmt->guardCondition->left.get());
                        bindingType->isReference = true;
                        bindingType->isMutable = false;
                        if (auto* owner = symbols.resolve(ownerName)) bindingType->origin = inferOrigin(owner, ownerName);
                        typePool.push_back(std::move(bindingType));
                        binding.type = typePool.back().get();
                        symbols.declare(binding.name, binding);
                        if (auto* owner = symbols.resolve(ownerName)) {
                            auto record = registerBorrow(owner, binding.type, ownerName);
                            if (auto* inserted = symbols.currentScope()->resolveLocal(binding.name)) inserted->memory.borrow = record;
                        }
                    } else {
                        typePool.push_back(std::move(bindingType));
                        binding.type = typePool.back().get();
                        symbols.declare(binding.name, binding);
                    }
                }
                checkStmt(stmt->guardBody.get());
                popCheckedScope();
            } catch (...) {
                popCheckedScope();
                throw;
            }
            break;
        }

        case StmtKind::ComptimeDecl: {
            if (!stmt->comptimeValue) {
                throw std::runtime_error("comptime declaration requires an initializer at line " + std::to_string(stmt->line));
            }
            // Local comptime values live in the current lexical scope and have no runtime
            // storage. Evaluate them now so later expressions can resolve them exactly like
            // global comptime declarations.
            ComptimeEvaluator evaluator(symbols);
            evaluator.evaluateAndDeclare(stmt);
            break;
        }
    }
}
