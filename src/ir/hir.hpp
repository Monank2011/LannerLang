#pragma once

#include "../parser/ast.hpp"
#include <cstdint>
#include <map>
#include <utility>
#include <string>
#include <vector>

namespace stable::hir {

enum class ScalarKind { Void, Bool, Int, Float, String, Aggregate };

enum class MemoryKind : std::uint8_t {
    Owned,
    SharedReference,
    ExclusiveReference,
    SharedView,
    ExclusiveView,
};

struct Type {
    ScalarKind kind = ScalarKind::Void;
    unsigned bits = 0;
    bool isSigned = false;
    std::string sourceName;
    std::string llvmName;
    std::vector<Type> members;
    std::uint64_t aggregateCount = 0;
    // Semantic memory state carried from the front end. This is metadata, not ABI.
    MemoryKind memory = MemoryKind::Owned;
    stable::memory::StorageOrigin origin;
    std::vector<stable::memory::StorageOrigin> nestedOrigins;

    Type() = default;
    Type(ScalarKind k, unsigned b, bool s, std::string source, std::string llvm,
         std::vector<Type> m = {}, std::uint64_t count = 0)
        : kind(k), bits(b), isSigned(s), sourceName(std::move(source)), llvmName(std::move(llvm)),
          members(std::move(m)), aggregateCount(count) {}

    std::string toString() const;
    bool operator==(const Type& other) const;
    bool operator!=(const Type& other) const { return !(*this == other); }
};

enum class Opcode {
    LoadLocal,
    StoreLocal,
    ConstInt,
    ConstString,
    ConstFloat,
    ConstBool,
    ZeroValue,
    Unary,
    Binary,
    Compare,
    Cast,
    Call,
    AddressOfLocal,
    AggregateIndex,
    LoadIndirect,
    StoreIndirect,
    ExtractValue,
    InsertValue,
    BoundsCheck,
    Branch,
    CondBranch,
    Return,
    DynamicArrayPush,
    ArenaCreate,
    DestroyLocal,
    DestroyAt,
};

struct Instruction {
    Opcode op = Opcode::ConstInt;
    Type type;
    std::string result;
    std::string slot;
    std::string lhs;
    std::string rhs;
    Type lhsType;
    Type rhsType;
    Type sourceType;
    std::string callee;
    std::string operatorName;
    std::string predicate;
    std::vector<std::string> args;
    std::uint64_t intValue = 0;
    double floatValue = 0.0;
    std::string stringValue;
    bool boolValue = false;
    std::string trueLabel;
    std::string falseLabel;
    std::string targetLabel;
    std::uint64_t aggregateIndex = 0;
    bool aggregateIndexConstant = false;
    std::uint64_t elementSize = 0;
};

struct AggregateDecl {
    std::string name;
    std::string body;
};

struct BasicBlock {
    std::string label;
    std::vector<Instruction> instructions;
};

struct Function {
    std::string name;
    Type returnType;
    std::vector<std::pair<std::string, Type>> params;
    std::map<std::string, Type> locals;
    std::vector<BasicBlock> blocks;
};

struct Module {
    std::vector<AggregateDecl> aggregates;
    std::vector<Function> functions;
};

Type lowerScalarType(const TypeNode* type, const std::map<std::string, EnumDecl*>& enums);
bool isSupportedScalarType(const TypeNode* type, const std::map<std::string, EnumDecl*>& enums);

std::string print(const Module& module);

} // namespace stable::hir
