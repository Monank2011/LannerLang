#include "hir.hpp"
#include <sstream>
#include <stdexcept>

namespace lanner::hir {

std::string Type::toString() const {
    if (!sourceName.empty()) return sourceName;
    if (kind == ScalarKind::Void) return "void";
    if (kind == ScalarKind::Bool) return "bool";
    if (kind == ScalarKind::String) return "string";
    if (kind == ScalarKind::Float) return "f" + std::to_string(bits);
    if (kind == ScalarKind::Aggregate) return "<aggregate>";
    return std::string(isSigned ? "i" : "u") + std::to_string(bits);
}

bool Type::operator==(const Type& other) const {
    return kind == other.kind && bits == other.bits && isSigned == other.isSigned &&
           sourceName == other.sourceName && llvmName == other.llvmName &&
           aggregateCount == other.aggregateCount && members == other.members;
}

Type lowerScalarType(const TypeNode* type, const std::map<std::string, EnumDecl*>& enums) {
    if (!type || type->isArray || type->isOptional || type->isReference || !type->generics.empty()) {
        throw std::runtime_error("HIR requires a plain scalar type");
    }
    if (type->name == "void") return {ScalarKind::Void, 0, false, type->name, ""};
    if (type->name == "bool") return {ScalarKind::Bool, 1, false, type->name, "i1"};
    if (type->name == "string") return {ScalarKind::String, 64, false, type->name, "ptr"};
    if (type->name == "f32") return {ScalarKind::Float, 32, true, type->name, "float"};
    if (type->name == "f64") return {ScalarKind::Float, 64, true, type->name, "double"};

    static const std::map<std::string, std::pair<unsigned, bool>> ints = {
        {"i8", {8, true}}, {"i16", {16, true}}, {"i32", {32, true}}, {"i64", {64, true}},
        {"isize", {64, true}}, {"u8", {8, false}}, {"u16", {16, false}}, {"u32", {32, false}},
        {"u64", {64, false}}, {"usize", {64, false}}
    };
    if (auto it = ints.find(type->name); it != ints.end()) {
        return {ScalarKind::Int, it->second.first, it->second.second, type->name,
                "i" + std::to_string(it->second.first)};
    }
    if (enums.count(type->name)) return {ScalarKind::Int, 32, true, type->name, "i32"};
    throw std::runtime_error("HIR does not support type '" + type->name + "' at line " + std::to_string(type->line));
}

bool isSupportedScalarType(const TypeNode* type, const std::map<std::string, EnumDecl*>& enums) {
    try {
        (void)lowerScalarType(type, enums);
        return true;
    } catch (...) {
        return false;
    }
}

static void printInst(std::ostringstream& out, const Instruction& i) {
    switch (i.op) {
        case Opcode::LoadLocal:
            out << "  " << i.result << " = load " << i.slot << " : " << i.type.toString() << '\n';
            break;
        case Opcode::StoreLocal:
            out << "  store " << i.lhs << " -> " << i.slot << " : " << i.type.toString() << '\n';
            break;
        case Opcode::ConstInt:
            out << "  " << i.result << " = const " << i.intValue << " : " << i.type.toString() << '\n';
            break;
        case Opcode::ConstString:
            out << "  " << i.result << " = string const (" << i.stringValue.size() << " bytes)\n";
            break;
        case Opcode::ConstFloat:
            out << "  " << i.result << " = const " << i.floatValue << " : " << i.type.toString() << '\n';
            break;
        case Opcode::ConstBool:
            out << "  " << i.result << " = const " << (i.boolValue ? "true" : "false") << " : bool\n";
            break;
        case Opcode::ZeroValue:
            out << "  " << i.result << " = zero : " << i.type.toString() << '\n';
            break;
        case Opcode::Unary:
            out << "  " << i.result << " = " << i.operatorName << ' ' << i.lhs << " : " << i.type.toString() << '\n';
            break;
        case Opcode::Binary:
        case Opcode::Compare:
            out << "  " << i.result << " = " << i.lhs << ' ' << i.operatorName << ' ' << i.rhs
                << " : " << i.type.toString() << '\n';
            break;
        case Opcode::Cast:
            out << "  " << i.result << " = cast " << i.lhs << " -> " << i.type.toString() << '\n';
            break;
        case Opcode::Call:
            out << "  " << i.result << " = call " << i.callee << '(';
            for (std::size_t n = 0; n < i.args.size(); ++n) {
                if (n) out << ", ";
                out << i.args[n];
            }
            out << ") : " << i.type.toString() << '\n';
            break;
        case Opcode::AddressOfLocal:
            out << "  " << i.result << " = addressof " << i.slot << " : ptr\n";
            break;
        case Opcode::AggregateIndex:
            out << "  " << i.result << " = elementaddr " << i.lhs;
            if (i.aggregateIndexConstant) out << "." << i.aggregateIndex;
            else out << "[" << i.rhs << "]";
            out << " : ptr\n";
            break;
        case Opcode::LoadIndirect:
            out << "  " << i.result << " = load *" << i.lhs << " : " << i.type.toString() << '\n';
            break;
        case Opcode::StoreIndirect:
            out << "  store " << i.rhs << " -> *" << i.lhs << " : " << i.type.toString() << '\n';
            break;
        case Opcode::ExtractValue:
            out << "  " << i.result << " = extract " << i.lhs << "." << i.aggregateIndex
                << " : " << i.type.toString() << '\n';
            break;
        case Opcode::InsertValue:
            out << "  " << i.result << " = insert " << i.rhs << " into " << i.lhs << "."
                << i.aggregateIndex << " : " << i.type.toString() << '\n';
            break;
        case Opcode::BoundsCheck:
            out << "  bounds " << i.rhs << " < " << i.aggregateIndex << '\n';
            break;
        case Opcode::Branch:
            out << "  br " << i.targetLabel << '\n';
            break;
        case Opcode::CondBranch:
            out << "  cbr " << i.lhs << " ? " << i.trueLabel << " : " << i.falseLabel << '\n';
            break;
        case Opcode::Return:
            if (i.lhs.empty()) out << "  ret void\n";
            else out << "  ret " << i.lhs << '\n';
            break;
        case Opcode::DynamicArrayPush:
            out << "  " << i.result << " = dyn.push " << i.lhs << ", " << i.rhs << "\n";
            break;
        case Opcode::ArenaCreate:
            out << "  " << i.result << " = arena.create " << i.lhs << "\n";
            break;
        case Opcode::DestroyLocal:
            out << "  destroy " << i.slot << " : " << i.type.toString() << '\n';
            break;
        case Opcode::DestroyAt:
            out << "  destroy *" << i.lhs << " : " << i.type.toString() << '\n';
            break;
    }
}

std::string print(const Module& module) {
    std::ostringstream out;
    for (const auto& aggregate : module.aggregates) {
        out << aggregate.name << " = type " << aggregate.body << '\n';
    }
    if (!module.aggregates.empty()) out << '\n';
    for (const auto& fn : module.functions) {
        out << "fn " << fn.name << '(';
        for (std::size_t i = 0; i < fn.params.size(); ++i) {
            if (i) out << ", ";
            out << fn.params[i].first << ": " << fn.params[i].second.toString();
        }
        out << ") -> " << fn.returnType.toString() << " {\n";
        for (const auto& block : fn.blocks) {
            out << block.label << ":\n";
            for (const auto& inst : block.instructions) printInst(out, inst);
        }
        out << "}\n\n";
    }
    return out.str();
}

} // namespace lanner::hir
