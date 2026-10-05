#include "hir_optimizer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace stable::hir {
namespace {

using Constant = std::variant<std::uint64_t, bool, double>;

std::uint64_t maskFor(unsigned bits) {
    if (bits >= 64) return std::numeric_limits<std::uint64_t>::max();
    return (std::uint64_t{1} << bits) - 1;
}

std::uint64_t normalized(std::uint64_t value, const Type& type) {
    if (type.kind != ScalarKind::Int) return value;
    return value & maskFor(type.bits);
}

std::int64_t asSigned(std::uint64_t value, unsigned bits) {
    value &= maskFor(bits);
    if (bits >= 64) return static_cast<std::int64_t>(value);
    const std::uint64_t sign = std::uint64_t{1} << (bits - 1);
    if ((value & sign) == 0) return static_cast<std::int64_t>(value);
    const std::uint64_t extended = value | (~maskFor(bits));
    return static_cast<std::int64_t>(extended);
}

bool constantFrom(const std::string& name, const std::map<std::string, Constant>& constants,
                  Constant& out) {
    const auto it = constants.find(name);
    if (it == constants.end()) return false;
    out = it->second;
    return true;
}

bool foldBinary(const Instruction& inst, const Constant& lhs, const Constant& rhs, Constant& result) {
    if (inst.lhsType.kind == ScalarKind::Bool || inst.lhsType.kind == ScalarKind::Void) {
        const auto* l = std::get_if<bool>(&lhs);
        const auto* r = std::get_if<bool>(&rhs);
        if (!l || !r) return false;
        if (inst.operatorName == "&&") result = *l && *r;
        else if (inst.operatorName == "||") result = *l || *r;
        else if (inst.operatorName == "==") result = *l == *r;
        else if (inst.operatorName == "!=") result = *l != *r;
        else return false;
        return true;
    }

    if (inst.lhsType.kind == ScalarKind::Float) {
        const auto* l = std::get_if<double>(&lhs);
        const auto* r = std::get_if<double>(&rhs);
        if (!l || !r) return false;

        const double lv = inst.lhsType.bits == 32 ? static_cast<double>(static_cast<float>(*l)) : *l;
        const double rv = inst.lhsType.bits == 32 ? static_cast<double>(static_cast<float>(*r)) : *r;
        const bool ordered = !std::isnan(lv) && !std::isnan(rv);
        if (inst.op == Opcode::Compare) {
            bool cmp = false;
            if (inst.operatorName == "==") cmp = ordered && lv == rv;
            else if (inst.operatorName == "!=") cmp = ordered && lv != rv;
            else if (inst.operatorName == "<") cmp = ordered && lv < rv;
            else if (inst.operatorName == ">") cmp = ordered && lv > rv;
            else if (inst.operatorName == "<=") cmp = ordered && lv <= rv;
            else if (inst.operatorName == ">=") cmp = ordered && lv >= rv;
            else return false;
            result = cmp;
            return true;
        }
        double value = 0.0;
        if (inst.operatorName == "+") value = lv + rv;
        else if (inst.operatorName == "-") value = lv - rv;
        else if (inst.operatorName == "*") value = lv * rv;
        else if (inst.operatorName == "/") value = lv / rv;
        else return false;
        if (inst.type.bits == 32) value = static_cast<double>(static_cast<float>(value));
        result = value;
        return true;
    }

    const auto* l = std::get_if<std::uint64_t>(&lhs);
    const auto* r = std::get_if<std::uint64_t>(&rhs);
    if (!l || !r || inst.lhsType.kind != ScalarKind::Int) return false;

    const Type& type = inst.lhsType;
    const unsigned bits = type.bits;
    const std::uint64_t lv = normalized(*l, type);
    const std::uint64_t rv = normalized(*r, type);
    const std::uint64_t mask = maskFor(bits);

    auto makeInt = [&](std::uint64_t value) {
        result = normalized(value, type);
        return true;
    };

    if (inst.op == Opcode::Compare) {
        bool cmp = false;
        if (inst.operatorName == "==") cmp = lv == rv;
        else if (inst.operatorName == "!=") cmp = lv != rv;
        else if (type.isSigned) {
            const auto ls = asSigned(lv, bits);
            const auto rs = asSigned(rv, bits);
            if (inst.operatorName == "<") cmp = ls < rs;
            else if (inst.operatorName == ">") cmp = ls > rs;
            else if (inst.operatorName == "<=") cmp = ls <= rs;
            else if (inst.operatorName == ">=") cmp = ls >= rs;
            else return false;
        } else {
            if (inst.operatorName == "<") cmp = lv < rv;
            else if (inst.operatorName == ">") cmp = lv > rv;
            else if (inst.operatorName == "<=") cmp = lv <= rv;
            else if (inst.operatorName == ">=") cmp = lv >= rv;
            else return false;
        }
        result = cmp;
        return true;
    }

    if (inst.operatorName == "+") return makeInt(lv + rv);
    if (inst.operatorName == "-") return makeInt(lv - rv);
    if (inst.operatorName == "*") return makeInt(lv * rv);
    if (inst.operatorName == "&") return makeInt(lv & rv);
    if (inst.operatorName == "|") return makeInt(lv | rv);
    if (inst.operatorName == "^") return makeInt(lv ^ rv);
    if (inst.operatorName == "/") {
        if (rv == 0) return false;
        if (type.isSigned) {
            const auto ls = asSigned(lv, bits);
            const auto rs = asSigned(rv, bits);
            const auto minSigned = bits == 64
                ? std::numeric_limits<std::int64_t>::min()
                : -(std::int64_t{1} << (bits - 1));
            if (ls == minSigned && rs == -1) return false;
            return makeInt(static_cast<std::uint64_t>(ls / rs));
        }
        return makeInt(lv / rv);
    }
    if (inst.operatorName == "%") {
        if (rv == 0) return false;
        if (type.isSigned) {
            const auto ls = asSigned(lv, bits);
            const auto rs = asSigned(rv, bits);
            const auto minSigned = bits == 64
                ? std::numeric_limits<std::int64_t>::min()
                : -(std::int64_t{1} << (bits - 1));
            if (ls == minSigned && rs == -1) return false;
            return makeInt(static_cast<std::uint64_t>(ls % rs));
        }
        return makeInt(lv % rv);
    }
    if (inst.operatorName == "<<" || inst.operatorName == ">>") {
        if (rv >= bits) return false;
        const unsigned amount = static_cast<unsigned>(rv);
        if (inst.operatorName == "<<") return makeInt((lv << amount) & mask);
        if (type.isSigned) return makeInt(static_cast<std::uint64_t>(asSigned(lv, bits) >> amount));
        return makeInt(lv >> amount);
    }
    return false;
}

bool foldUnary(const Instruction& inst, const Constant& input, Constant& result) {
    if (inst.type.kind == ScalarKind::Bool) {
        const auto* v = std::get_if<bool>(&input);
        if (!v || inst.operatorName != "!") return false;
        result = !*v;
        return true;
    }
    if (inst.type.kind == ScalarKind::Float) {
        const auto* v = std::get_if<double>(&input);
        if (!v) return false;
        if (inst.operatorName == "+") { result = *v; return true; }
        if (inst.operatorName == "-") {
            double value = -*v;
            if (inst.type.bits == 32) value = static_cast<double>(static_cast<float>(value));
            result = value;
            return true;
        }
        return false;
    }
    if (inst.type.kind != ScalarKind::Int) return false;
    const auto* v = std::get_if<std::uint64_t>(&input);
    if (!v) return false;
    if (inst.operatorName == "+") { result = normalized(*v, inst.type); return true; }
    if (inst.operatorName == "-") { result = normalized(0 - *v, inst.type); return true; }
    if (inst.operatorName == "~") { result = normalized(~*v, inst.type); return true; }
    return false;
}

bool foldCast(const Instruction& inst, const Constant& input, Constant& result) {
    if (inst.sourceType.kind == ScalarKind::Bool && inst.type.kind == ScalarKind::Bool) {
        const auto* v = std::get_if<bool>(&input);
        if (!v) return false;
        result = *v; return true;
    }
    if (inst.sourceType.kind == ScalarKind::Bool && inst.type.kind == ScalarKind::Int) {
        const auto* v = std::get_if<bool>(&input);
        if (!v) return false;
        result = normalized(*v ? 1U : 0U, inst.type); return true;
    }
    if (inst.sourceType.kind == ScalarKind::Int && inst.type.kind == ScalarKind::Bool) {
        const auto* v = std::get_if<std::uint64_t>(&input);
        if (!v) return false;
        result = normalized(*v, inst.sourceType) != 0; return true;
    }
    if (inst.sourceType.kind == ScalarKind::Int && inst.type.kind == ScalarKind::Int) {
        const auto* v = std::get_if<std::uint64_t>(&input);
        if (!v) return false;
        if (inst.sourceType.isSigned && inst.type.bits > inst.sourceType.bits) {
            result = static_cast<std::uint64_t>(asSigned(*v, inst.sourceType.bits));
        } else {
            result = normalized(*v, inst.type);
        }
        return true;
    }
    if (inst.sourceType.kind == ScalarKind::Int && inst.type.kind == ScalarKind::Float) {
        const auto* v = std::get_if<std::uint64_t>(&input);
        if (!v) return false;
        double value = inst.sourceType.isSigned
            ? static_cast<double>(asSigned(*v, inst.sourceType.bits))
            : static_cast<double>(normalized(*v, inst.sourceType));
        if (inst.type.bits == 32) value = static_cast<double>(static_cast<float>(value));
        result = value;
        return true;
    }
    if (inst.sourceType.kind == ScalarKind::Float && inst.type.kind == ScalarKind::Float) {
        const auto* v = std::get_if<double>(&input);
        if (!v) return false;
        double value = *v;
        if (inst.type.bits == 32) value = static_cast<double>(static_cast<float>(value));
        result = value;
        return true;
    }
    // Deliberately do not fold float->integer conversions here. LLVM treats
    // out-of-range/NaN conversions as poison, and reproducing those exact
    // semantics safely requires range checks beyond a simple host cast.
    return false;
}

void replaceWithConstant(Instruction& inst, const Constant& value) {
    if (const auto* i = std::get_if<std::uint64_t>(&value)) {
        inst.op = Opcode::ConstInt;
        inst.intValue = normalized(*i, inst.type);
        inst.lhs.clear(); inst.rhs.clear(); inst.args.clear();
        return;
    }
    if (const auto* f = std::get_if<double>(&value)) {
        inst.op = Opcode::ConstFloat;
        inst.floatValue = inst.type.bits == 32 ? static_cast<double>(static_cast<float>(*f)) : *f;
        inst.lhs.clear(); inst.rhs.clear(); inst.args.clear();
        return;
    }
    inst.op = Opcode::ConstBool;
    inst.boolValue = std::get<bool>(value);
    inst.lhs.clear(); inst.rhs.clear(); inst.args.clear();
}

bool isPure(const Instruction& inst) {
    switch (inst.op) {
        case Opcode::LoadLocal:
        case Opcode::ConstInt:
        case Opcode::ConstFloat:
        case Opcode::ConstBool:
        case Opcode::Unary:
        case Opcode::Binary:
        case Opcode::Compare:
        case Opcode::Cast:
            return !inst.result.empty();
        default:
            return false;
    }
}

void constantFoldBlock(BasicBlock& bb) {
    std::map<std::string, Constant> constants;
    for (auto& inst : bb.instructions) {
        if (inst.op == Opcode::ConstInt) constants[inst.result] = normalized(inst.intValue, inst.type);
        else if (inst.op == Opcode::ConstBool) constants[inst.result] = inst.boolValue;
        else if (inst.op == Opcode::ConstFloat) {
            double value = inst.floatValue;
            if (inst.type.bits == 32) value = static_cast<double>(static_cast<float>(value));
            constants[inst.result] = value;
        }
        else if (inst.op == Opcode::LoadLocal) constants.erase(inst.result);
        else if (inst.op == Opcode::StoreLocal) {
            // A store changes the memory state represented by future LoadLocal instructions.
            constants.clear();
        } else if (inst.op == Opcode::Call) {
            if (!inst.result.empty()) constants.erase(inst.result);
        } else if (inst.op == Opcode::Unary) {
            Constant input, folded;
            if (constantFrom(inst.lhs, constants, input) && foldUnary(inst, input, folded)) {
                replaceWithConstant(inst, folded);
                constants[inst.result] = folded;
            } else constants.erase(inst.result);
        } else if (inst.op == Opcode::Binary || inst.op == Opcode::Compare) {
            Constant lhs, rhs, folded;
            if (constantFrom(inst.lhs, constants, lhs) && constantFrom(inst.rhs, constants, rhs) &&
                foldBinary(inst, lhs, rhs, folded)) {
                replaceWithConstant(inst, folded);
                constants[inst.result] = folded;
            } else constants.erase(inst.result);
        } else if (inst.op == Opcode::Cast) {
            Constant input, folded;
            if (constantFrom(inst.lhs, constants, input) && foldCast(inst, input, folded)) {
                replaceWithConstant(inst, folded);
                constants[inst.result] = folded;
            } else constants.erase(inst.result);
        }

        if (inst.op == Opcode::Branch) {
            // Nothing to fold.
        } else if (inst.op == Opcode::CondBranch) {
            Constant condition;
            if (constantFrom(inst.lhs, constants, condition)) {
                if (const auto* value = std::get_if<bool>(&condition)) {
                    inst.op = Opcode::Branch;
                    inst.targetLabel = *value ? inst.trueLabel : inst.falseLabel;
                    inst.lhs.clear();
                    inst.trueLabel.clear();
                    inst.falseLabel.clear();
                }
            }
        }
    }
}

void removeUnreachable(Function& fn) {
    if (fn.blocks.empty()) return;
    std::map<std::string, std::size_t> index;
    for (std::size_t i = 0; i < fn.blocks.size(); ++i) index[fn.blocks[i].label] = i;

    std::set<std::size_t> reachable;
    std::vector<std::size_t> work{0};
    while (!work.empty()) {
        const auto current = work.back(); work.pop_back();
        if (!reachable.insert(current).second) continue;
        const auto& instructions = fn.blocks[current].instructions;
        if (instructions.empty()) continue;
        const auto& term = instructions.back();
        if (term.op == Opcode::Branch) {
            auto it = index.find(term.targetLabel);
            if (it != index.end()) work.push_back(it->second);
        } else if (term.op == Opcode::CondBranch) {
            auto a = index.find(term.trueLabel); auto b = index.find(term.falseLabel);
            if (a != index.end()) work.push_back(a->second);
            if (b != index.end()) work.push_back(b->second);
        }
    }

    std::vector<BasicBlock> kept;
    kept.reserve(reachable.size());
    for (std::size_t i = 0; i < fn.blocks.size(); ++i) if (reachable.count(i) != 0) kept.push_back(std::move(fn.blocks[i]));
    fn.blocks = std::move(kept);
}

void eliminateRedundantLocalLoads(Function& fn) {
    // Forward a direct StoreLocal into LoadLocal only when every use of the
    // loaded SSA value stays in the same basic block. Calls and memory operations
    // conservatively invalidate the fact because an address may have escaped.
    // We rewrite uses directly instead of inventing a runtime copy opcode.
    std::map<std::string, std::set<std::size_t>> useBlocks;
    for (std::size_t blockIndex = 0; blockIndex < fn.blocks.size(); ++blockIndex) {
        for (const auto& inst : fn.blocks[blockIndex].instructions) {
            if (!inst.lhs.empty()) useBlocks[inst.lhs].insert(blockIndex);
            if (!inst.rhs.empty()) useBlocks[inst.rhs].insert(blockIndex);
            for (const auto& arg : inst.args) if (!arg.empty()) useBlocks[arg].insert(blockIndex);
        }
    }

    for (std::size_t blockIndex = 0; blockIndex < fn.blocks.size(); ++blockIndex) {
        auto& bb = fn.blocks[blockIndex];
        std::map<std::string, std::string> available;
        std::map<std::string, std::string> replace;
        std::vector<Instruction> kept;
        kept.reserve(bb.instructions.size());

        auto resolve = [&](std::string value) {
            std::set<std::string> seen;
            while (!value.empty()) {
                if (!seen.insert(value).second) break;
                auto it = replace.find(value);
                if (it == replace.end()) break;
                value = it->second;
            }
            return value;
        };

        auto rewriteInstructionUses = [&](Instruction& inst) {
            if (!inst.lhs.empty()) inst.lhs = resolve(inst.lhs);
            if (!inst.rhs.empty()) inst.rhs = resolve(inst.rhs);
            for (auto& arg : inst.args) if (!arg.empty()) arg = resolve(arg);
        };

        auto invalidateAll = [&]() {
            available.clear();
        };

        for (auto inst : bb.instructions) {
            rewriteInstructionUses(inst);

            if (inst.op == Opcode::LoadLocal && !inst.result.empty()) {
                auto it = available.find(inst.slot);
                const auto usesIt = useBlocks.find(inst.result);
                const bool sameBlockUses = usesIt != useBlocks.end() &&
                    usesIt->second.size() == 1 && usesIt->second.count(blockIndex) != 0;
                // Never forward aggregate loads blindly: aggregates may carry an owning
                // resource or a borrow, and replacing the load can duplicate an SSA
                // ownership edge that the memory/drop analysis must keep explicit.
                const bool copySafeType = inst.type.kind != ScalarKind::Aggregate;
                if (it != available.end() && !it->second.empty() && sameBlockUses && copySafeType) {
                    replace[inst.result] = it->second;
                    continue;
                }
                available.erase(inst.slot);
                kept.push_back(std::move(inst));
                continue;
            }

            if (inst.op == Opcode::StoreLocal) {
                if (!inst.lhs.empty()) available[inst.slot] = inst.lhs;
                else available.erase(inst.slot);
                kept.push_back(std::move(inst));
                continue;
            }

            if (inst.op == Opcode::Call || inst.op == Opcode::StoreIndirect ||
                inst.op == Opcode::DynamicArrayPush || inst.op == Opcode::DestroyLocal ||
                inst.op == Opcode::DestroyAt) {
                invalidateAll();
            }

            kept.push_back(std::move(inst));
        }
        bb.instructions = std::move(kept);
    }
}

void eliminateProvablySafeBoundsChecks(BasicBlock& bb) {
    std::map<std::string, std::uint64_t> constants;
    std::vector<Instruction> kept;
    kept.reserve(bb.instructions.size());

    for (auto inst : bb.instructions) {
        if (inst.op == Opcode::ConstInt && !inst.result.empty()) {
            constants[inst.result] = normalized(inst.intValue, inst.type);
        } else if (inst.op == Opcode::ConstBool) {
            constants.erase(inst.result);
        } else if (inst.op == Opcode::BoundsCheck) {
            const auto it = constants.find(inst.rhs);
            // Fixed-size arrays carry their exact bound directly in the HIR.
            // Only remove the check when the index is a known unsigned value
            // strictly inside that bound. A signed constant is also safe when
            // its mathematical value is non-negative.
            if (it != constants.end() && inst.aggregateIndexConstant) {
                const std::uint64_t raw = it->second;
                bool nonNegative = true;
                if (inst.rhsType.kind == ScalarKind::Int && inst.rhsType.isSigned) {
                    nonNegative = asSigned(raw, inst.rhsType.bits) >= 0;
                }
                if (nonNegative && raw < inst.aggregateIndex) continue;
            }
        } else if (inst.op == Opcode::StoreLocal || inst.op == Opcode::LoadLocal ||
                   inst.op == Opcode::Call || inst.op == Opcode::StoreIndirect ||
                   inst.op == Opcode::DynamicArrayPush) {
            // A value's identity remains stable for HIR SSA names, but the
            // instruction can change what a later bounds proof would mean.
            // Constant facts are kept only for literal-producing values, so no
            // additional invalidation is needed here.
        }
        if (inst.op == Opcode::Unary || inst.op == Opcode::Binary ||
            inst.op == Opcode::Compare || inst.op == Opcode::Cast ||
            inst.op == Opcode::LoadLocal || inst.op == Opcode::Call) {
            if (!inst.result.empty()) constants.erase(inst.result);
        }
        kept.push_back(std::move(inst));
    }
    bb.instructions = std::move(kept);
}

void eliminateDeadPureValues(Function& fn) {
    std::map<std::string, int> uses;
    for (const auto& bb : fn.blocks) {
        for (const auto& inst : bb.instructions) {
            if (!inst.lhs.empty()) ++uses[inst.lhs];
            if (!inst.rhs.empty()) ++uses[inst.rhs];
            for (const auto& arg : inst.args) ++uses[arg];
        }
    }
    for (auto& bb : fn.blocks) {
        std::vector<Instruction> kept;
        kept.reserve(bb.instructions.size());
        for (auto& inst : bb.instructions) {
            if (isPure(inst) && inst.result.size() > 0 && uses[inst.result] == 0) continue;
            kept.push_back(std::move(inst));
        }
        bb.instructions = std::move(kept);
    }
}

} // namespace

void optimize(Module& module, OptimizationLevel level) {
    if (level == OptimizationLevel::O0) return;
    for (auto& fn : module.functions) {
        for (auto& bb : fn.blocks) {
            constantFoldBlock(bb);
            if (level == OptimizationLevel::O2) eliminateProvablySafeBoundsChecks(bb);
        }
        if (level == OptimizationLevel::O2) {
            eliminateRedundantLocalLoads(fn);
            removeUnreachable(fn);
            eliminateDeadPureValues(fn);
        }
    }
}

} // namespace stable::hir
