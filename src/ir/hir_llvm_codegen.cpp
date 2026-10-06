#include "hir_llvm_codegen.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace lanner::hir {

std::string LLVMCodegen::llvmType(const Type& type) const {
    switch (type.kind) {
        case ScalarKind::Void: return "void";
        case ScalarKind::Bool: return "i1";
        case ScalarKind::Int: return "i" + std::to_string(type.bits);
        case ScalarKind::Float: return type.bits == 32 ? "float" : "double";
        case ScalarKind::String: return "ptr";
        case ScalarKind::Aggregate:
            if (!type.llvmName.empty()) return type.llvmName;
            throw std::runtime_error("aggregate HIR type has no LLVM spelling");
    }
    throw std::runtime_error("unknown HIR type");
}

std::string LLVMCodegen::cmpPredicate(const Type& type, const std::string& op) const {
    if (type.kind == ScalarKind::Float) {
        if (op == "==") return "oeq";
        if (op == "!=") return "one";
        if (op == "<") return "olt";
        if (op == ">") return "ogt";
        if (op == "<=") return "ole";
        if (op == ">=") return "oge";
    } else {
        const bool signedness = type.kind == ScalarKind::Int && type.isSigned;
        const char* p = signedness ? "s" : "u";
        if (op == "==") return "eq";
        if (op == "!=") return "ne";
        if (op == "<") return std::string(p) + "lt";
        if (op == ">") return std::string(p) + "gt";
        if (op == "<=") return std::string(p) + "le";
        if (op == ">=") return std::string(p) + "ge";
    }
    throw std::runtime_error("unsupported HIR comparison '" + op + "'");
}

std::string LLVMCodegen::binaryOpcode(const Type& type, const std::string& op) const {
    if (type.kind == ScalarKind::Float) {
        if (op == "+") return "fadd";
        if (op == "-") return "fsub";
        if (op == "*") return "fmul";
        if (op == "/") return "fdiv";
    } else {
        if (op == "+") return "add";
        if (op == "-") return "sub";
        if (op == "*") return "mul";
        if (op == "/") return type.isSigned ? "sdiv" : "udiv";
        if (op == "%") return type.isSigned ? "srem" : "urem";
        if (op == "&") return "and";
        if (op == "|") return "or";
        if (op == "^") return "xor";
        if (op == "<<") return "shl";
        if (op == ">>") return type.isSigned ? "ashr" : "lshr";
    }
    throw std::runtime_error("unsupported HIR binary operator '" + op + "'");
}

std::string LLVMCodegen::castInstruction(const Type& source, const Type& dest) const {
    if (source == dest) return "identity";
    if (source.kind == ScalarKind::Int && dest.kind == ScalarKind::Int) {
        if (source.bits == dest.bits) return "bitcast";
        if (source.bits < dest.bits) return source.isSigned ? "sext" : "zext";
        return "trunc";
    }
    if (source.kind == ScalarKind::Int && dest.kind == ScalarKind::Float) {
        return source.isSigned ? "sitofp" : "uitofp";
    }
    if (source.kind == ScalarKind::Float && dest.kind == ScalarKind::Int) {
        return dest.isSigned ? "fptosi" : "fptoui";
    }
    if (source.kind == ScalarKind::Float && dest.kind == ScalarKind::Float) {
        if (source.bits == 32 && dest.bits == 64) return "fpext";
        if (source.bits == 64 && dest.bits == 32) return "fptrunc";
    }
    if (source.kind == ScalarKind::Bool && dest.kind == ScalarKind::Int) return "zext";
    if (source.kind == ScalarKind::Int && dest.kind == ScalarKind::Bool) return "trunc";
    if (source.kind == ScalarKind::Bool && dest.kind == ScalarKind::Bool) return "bitcast";
    throw std::runtime_error("unsupported HIR cast " + source.toString() + " -> " + dest.toString());
}

void LLVMCodegen::emitFunction(const Module& module, const Function& fn, std::string& out) const {
    out += "define " + llvmType(fn.returnType) + " @" + fn.name + "(";
    for (std::size_t i = 0; i < fn.params.size(); ++i) {
        if (i) out += ", ";
        out += llvmType(fn.params[i].second) + " %arg" + std::to_string(i);
    }
    out += ") {\n";

    out += "entry:\n";
    for (const auto& [slot, type] : fn.locals) {
        const std::string align = (type.kind == ScalarKind::Aggregate || type.kind == ScalarKind::String || type.bits >= 64) ? "8" : "4";
        out += "  %slot_" + slot + " = alloca " + llvmType(type) + ", align " + align + "\n";
    }

    std::map<std::string, std::size_t> paramIndex;
    for (std::size_t i = 0; i < fn.params.size(); ++i) paramIndex[fn.params[i].first] = i;
    for (const auto& [slot, type] : fn.locals) {
        auto it = paramIndex.find(slot);
        if (it == paramIndex.end()) continue;
        out += "  store " + llvmType(type) + " %arg" + std::to_string(it->second) + ", ptr %slot_" + slot + "\n";
    }

    int internalCounter = 0;
    const auto emitBlockInstructions = [&](const BasicBlock& block, std::string& text) {
        std::function<bool(const Type&)> ownsResources = [&](const Type& t) -> bool {
            if (t.llvmName == "%LannerArena" || t.llvmName == "%LannerDynArray") return true;
            if (t.sourceName == "View" || t.sourceName == "EditView" || t.llvmName == "ptr" || t.kind != ScalarKind::Aggregate) return false;
            for (const auto& member : t.members) if (ownsResources(member)) return true;
            return t.sourceName == "Result" || (!t.sourceName.empty() && t.sourceName.back() == '?') || t.sourceName == "array";
        };

        // Emit ownership cleanup and return the label that remains the active
        // continuation block after the cleanup has completed. This matters when
        // one owned aggregate contains another: a recursive drop can introduce
        // branches and joins, so the next drop must use that join as its predecessor.
        std::function<std::string(const Type&, const std::string&, const std::string&)> destroyAt;
        destroyAt = [&](const Type& t, const std::string& addr, const std::string& currentLabel) -> std::string {
            if (!ownsResources(t)) return currentLabel;

            if (t.llvmName == "%LannerArena") {
                text += "  call void @__lanner_arena_destroy(ptr " + addr + ")\n";
                return currentLabel;
            }

            if (t.llvmName == "%LannerDynArray") {
                if (t.members.empty()) throw std::runtime_error("dynamic array HIR type has no element type");
                const Type& elem = t.members[0];
                const std::string done = "hir.drop.done." + std::to_string(internalCounter++);
                const std::string arr = "%drop_arr_" + std::to_string(internalCounter++);
                const std::string data = "%drop_data_" + std::to_string(internalCounter++);
                const std::string len = "%drop_len_" + std::to_string(internalCounter++);
                const std::string arena = "%drop_arena_" + std::to_string(internalCounter++);
                text += "  " + arr + " = load %LannerDynArray, ptr " + addr + "\n";
                text += "  " + data + " = extractvalue %LannerDynArray " + arr + ", 0\n";
                text += "  " + len + " = extractvalue %LannerDynArray " + arr + ", 1\n";
                text += "  " + arena + " = extractvalue %LannerDynArray " + arr + ", 3\n";

                std::string continuation;
                const bool elemOwns = ownsResources(elem);
                if (elemOwns) {
                    const std::string hasData = "%drop_hasdata_" + std::to_string(internalCounter++);
                    const std::string loop = "hir.drop.array.loop." + std::to_string(internalCounter++);
                    const std::string body = "hir.drop.array.body." + std::to_string(internalCounter++);
                    const std::string step = "hir.drop.array.step." + std::to_string(internalCounter++);
                    text += "  " + hasData + " = icmp ne ptr " + data + ", null\n";
                    text += "  br i1 " + hasData + ", label %" + loop + ", label %" + done + "\n";
                    text += loop + ":\n";
                    const std::string idx = "%drop_idx_" + std::to_string(internalCounter++);
                    const std::string next = "%drop_next_" + std::to_string(internalCounter++);
                    const std::string active = "%drop_active_" + std::to_string(internalCounter++);
                    text += "  " + idx + " = phi i64 [ 0, %" + currentLabel + " ], [ " + next + ", %" + step + " ]\n";
                    text += "  " + active + " = icmp ult i64 " + idx + ", " + len + "\n";
                    text += "  br i1 " + active + ", label %" + body + ", label %" + done + "\n";
                    text += body + ":\n";
                    const std::string eptr = "%drop_elem_ptr_" + std::to_string(internalCounter++);
                    text += "  " + eptr + " = getelementptr inbounds " + llvmType(elem) + ", ptr " + data + ", i64 " + idx + "\n";
                    const std::string childContinuation = destroyAt(elem, eptr, body);
                    text += "  br label %" + step + "\n";
                    text += step + ":\n";
                    text += "  " + next + " = add i64 " + idx + ", 1\n";
                    text += "  br label %" + loop + "\n";
                    (void)childContinuation;
                    continuation = done;
                } else {
                    text += "  br label %" + done + "\n";
                    continuation = done;
                }

                text += continuation + ":\n";
                const std::string hasArena = "%drop_hasarena_" + std::to_string(internalCounter++);
                const std::string hasData2 = "%drop_hasdata2_" + std::to_string(internalCounter++);
                const std::string noArena = "%drop_noarena_" + std::to_string(internalCounter++);
                const std::string canFree = "%drop_canfree_" + std::to_string(internalCounter++);
                const std::string freeIt = "hir.drop.array.free." + std::to_string(internalCounter++);
                const std::string skip = "hir.drop.array.skip." + std::to_string(internalCounter++);
                text += "  " + hasArena + " = icmp ne ptr " + arena + ", null\n";
                text += "  " + hasData2 + " = icmp ne ptr " + data + ", null\n";
                text += "  " + noArena + " = xor i1 true, " + hasArena + "\n";
                text += "  " + canFree + " = and i1 " + hasData2 + ", " + noArena + "\n";
                text += "  br i1 " + canFree + ", label %" + freeIt + ", label %" + skip + "\n";
                text += freeIt + ":\n";
                text += "  call void @free(ptr " + data + ")\n";
                text += "  br label %" + skip + "\n";
                text += skip + ":\n";
                return skip;
            }

            if (!t.members.empty() && t.sourceName == "Result") {
                const std::string value = "%drop_result_" + std::to_string(internalCounter++);
                const std::string tag = "%drop_result_tag_" + std::to_string(internalCounter++);
                text += "  " + value + " = load " + llvmType(t) + ", ptr " + addr + "\n";
                text += "  " + tag + " = extractvalue " + llvmType(t) + " " + value + ", 0\n";
                const std::string ok = "hir.drop.result.ok." + std::to_string(internalCounter++);
                const std::string err = "hir.drop.result.err." + std::to_string(internalCounter++);
                const std::string merge = "hir.drop.result.merge." + std::to_string(internalCounter++);
                text += "  br i1 " + tag + ", label %" + ok + ", label %" + err + "\n";
                text += ok + ":\n";
                const std::string okTmp = "%drop_result_ok_tmp_" + std::to_string(internalCounter++);
                const std::string okVal = "%drop_result_ok_" + std::to_string(internalCounter++);
                text += "  " + okTmp + " = alloca " + llvmType(t.members[0]) + ", align 8\n";
                text += "  " + okVal + " = extractvalue " + llvmType(t) + " " + value + ", 1\n";
                text += "  store " + llvmType(t.members[0]) + " " + okVal + ", ptr " + okTmp + "\n";
                const std::string okContinuation = destroyAt(t.members[0], okTmp, ok);
                text += "  br label %" + merge + "\n";
                (void)okContinuation;
                text += err + ":\n";
                const std::string errTmp = "%drop_result_err_tmp_" + std::to_string(internalCounter++);
                const std::string errVal = "%drop_result_err_" + std::to_string(internalCounter++);
                text += "  " + errTmp + " = alloca " + llvmType(t.members[1]) + ", align 8\n";
                text += "  " + errVal + " = extractvalue " + llvmType(t) + " " + value + ", 2\n";
                text += "  store " + llvmType(t.members[1]) + " " + errVal + ", ptr " + errTmp + "\n";
                const std::string errContinuation = destroyAt(t.members[1], errTmp, err);
                text += "  br label %" + merge + "\n";
                (void)errContinuation;
                text += merge + ":\n";
                return merge;
            }

            if (!t.members.empty() && !t.sourceName.empty() && t.sourceName.back() == '?') {
                const std::string value = "%drop_opt_" + std::to_string(internalCounter++);
                const std::string tag = "%drop_opt_tag_" + std::to_string(internalCounter++);
                const std::string payload = "%drop_opt_payload_" + std::to_string(internalCounter++);
                text += "  " + value + " = load " + llvmType(t) + ", ptr " + addr + "\n";
                text += "  " + tag + " = extractvalue " + llvmType(t) + " " + value + ", 0\n";
                const std::string has = "hir.drop.opt.has." + std::to_string(internalCounter++);
                const std::string merge = "hir.drop.opt.merge." + std::to_string(internalCounter++);
                text += "  br i1 " + tag + ", label %" + has + ", label %" + merge + "\n";
                text += has + ":\n";
                text += "  " + payload + " = alloca " + llvmType(t.members[0]) + ", align 8\n";
                const std::string payloadValue = "%drop_opt_value_" + std::to_string(internalCounter++);
                text += "  " + payloadValue + " = extractvalue " + llvmType(t) + " " + value + ", 1\n";
                text += "  store " + llvmType(t.members[0]) + " " + payloadValue + ", ptr " + payload + "\n";
                const std::string payloadContinuation = destroyAt(t.members[0], payload, has);
                text += "  br label %" + merge + "\n";
                (void)payloadContinuation;
                text += merge + ":\n";
                return merge;
            }

            if (t.llvmName.size() && t.llvmName.front() == '[') {
                const std::size_t count = static_cast<std::size_t>(t.aggregateCount);
                std::string continuation = currentLabel;
                for (std::size_t i = 0; i < count; ++i) {
                    const std::string field = "%drop_array_elem_" + std::to_string(internalCounter++);
                    text += "  " + field + " = getelementptr inbounds " + llvmType(t) + ", ptr " + addr + ", i64 0, i64 " + std::to_string(i) + "\n";
                    continuation = destroyAt(t.members[0], field, continuation);
                }
                return continuation;
            }

            if (!t.members.empty() && t.sourceName != "View" && t.sourceName != "EditView") {
                std::string continuation = currentLabel;
                for (std::size_t i = 0; i < t.members.size(); ++i) {
                    const std::string field = "%drop_struct_field_" + std::to_string(internalCounter++);
                    text += "  " + field + " = getelementptr inbounds " + llvmType(t) + ", ptr " + addr + ", i32 0, i32 " + std::to_string(i) + "\n";
                    continuation = destroyAt(t.members[i], field, continuation);
                }
                return continuation;
            }
            return currentLabel;
        };

        std::string currentFlowLabel = block.label;
        for (const auto& inst : block.instructions) {
            const std::string type = llvmType(inst.type);
            switch (inst.op) {
                case Opcode::LoadLocal:
                    text += "  " + inst.result + " = load " + type + ", ptr %slot_" + inst.slot + "\n";
                    break;
                case Opcode::StoreLocal:
                    text += "  store " + type + " " + inst.lhs + ", ptr %slot_" + inst.slot + "\n";
                    break;
                case Opcode::ConstInt:
                    text += "  " + inst.result + " = add " + type + " 0, " + std::to_string(inst.intValue) + "\n";
                    break;
                case Opcode::ConstString: {
                    if (inst.type.kind != ScalarKind::String) throw std::runtime_error("string HIR constant has non-string type");
                    const std::string global = "@.lanner.str." + fn.name + "." + (inst.result.empty() ? "anon" : inst.result.substr(1));
                    text += "  " + inst.result + " = getelementptr inbounds [" + std::to_string(inst.stringValue.size() + 1) + " x i8], ptr " + global + ", i64 0, i64 0\n";
                    break;
                }
                case Opcode::ConstFloat:
                    text += "  " + inst.result + " = fadd " + type + " 0.0, " + std::to_string(inst.floatValue) + "\n";
                    break;
                case Opcode::ConstBool:
                    text += "  " + inst.result + " = xor i1 false, " + std::string(inst.boolValue ? "true" : "false") + "\n";
                    break;
                case Opcode::ZeroValue:
                    if (inst.type.kind == ScalarKind::Bool) {
                        text += "  " + inst.result + " = xor i1 false, false\n";
                    } else if (inst.type.kind == ScalarKind::Int) {
                        text += "  " + inst.result + " = add " + type + " 0, 0\n";
                    } else if (inst.type.kind == ScalarKind::Float) {
                        text += "  " + inst.result + " = fadd " + type + " 0.0, 0.0\n";
                    } else {
                        text += "  " + inst.result + " = select i1 true, " + type + " zeroinitializer, " + type + " zeroinitializer\n";
                    }
                    break;
                case Opcode::Unary:
                    if (inst.operatorName == "+") text += "  " + inst.result + " = add " + type + " 0, " + inst.lhs + "\n";
                    else if (inst.operatorName == "-") text += "  " + inst.result + " = sub " + type + " 0, " + inst.lhs + "\n";
                    else if (inst.operatorName == "~") text += "  " + inst.result + " = xor " + type + " -1, " + inst.lhs + "\n";
                    else if (inst.operatorName == "!") text += "  " + inst.result + " = xor i1 true, " + inst.lhs + "\n";
                    else throw std::runtime_error("unsupported HIR unary operator '" + inst.operatorName + "'");
                    break;
                case Opcode::Binary:
                    text += "  " + inst.result + " = " + binaryOpcode(inst.lhsType, inst.operatorName) + " " + llvmType(inst.lhsType) + " " + inst.lhs + ", " + inst.rhs + "\n";
                    break;
                case Opcode::Compare:
                    text += "  " + inst.result + " = " +
                            (inst.lhsType.kind == ScalarKind::Float ? "fcmp " : "icmp ") +
                            cmpPredicate(inst.lhsType, inst.operatorName) + " " + llvmType(inst.lhsType) +
                            " " + inst.lhs + ", " + inst.rhs + "\n";
                    break;
                case Opcode::Cast: {
                    const std::string castOp = castInstruction(inst.sourceType, inst.type);
                    if (castOp == "identity") {
                        if (inst.type.kind == ScalarKind::Float) text += "  " + inst.result + " = fadd " + type + " 0.0, " + inst.lhs + "\n";
                        else if (inst.type.kind == ScalarKind::Bool) text += "  " + inst.result + " = xor i1 false, " + inst.lhs + "\n";
                        else text += "  " + inst.result + " = add " + type + " 0, " + inst.lhs + "\n";
                    } else {
                        text += "  " + inst.result + " = " + castOp + " " + llvmType(inst.sourceType) + " " + inst.lhs + " to " + type + "\n";
                    }
                    break;
                }
                case Opcode::Call: {
                    struct Signature { std::string ret; std::vector<std::string> params; };
                    std::optional<Signature> runtime;
                    if (inst.callee == "llvm.trap") runtime = Signature{"void", {}};
                    else if (inst.callee == "__lanner_read_file") runtime = Signature{"%LannerDynArray", {"ptr"}};
                    else if (inst.callee == "__lanner_write_stdout") runtime = Signature{"void", {"ptr"}};
                    else if (inst.callee == "__lanner_write_raw") runtime = Signature{"void", {"ptr"}};
                    else if (inst.callee == "__lanner_write_i64_raw") runtime = Signature{"void", {"i64"}};
                    else if (inst.callee == "__lanner_write_byte_raw") runtime = Signature{"void", {"i64"}};
                    else if (inst.callee == "__lanner_print_i64") runtime = Signature{"void", {"i64"}};
                    else if (inst.callee == "__lanner_print_u64") runtime = Signature{"void", {"i64"}};
                    else if (inst.callee == "__lanner_print_f64") runtime = Signature{"void", {"double"}};
                    else if (inst.callee == "__lanner_print_bool") runtime = Signature{"void", {"i1"}};
                    else if (inst.callee == "__lanner_print_string") runtime = Signature{"void", {"ptr"}};
                    else if (inst.callee == "__lanner_string_len") runtime = Signature{"i64", {"ptr"}};
                    else if (inst.callee == "__lanner_getenv") runtime = Signature{"ptr", {"ptr"}};

                    std::vector<std::string> paramTypes;
                    std::string returnType = type;
                    if (runtime) {
                        if (runtime->params.size() != inst.args.size()) {
                            throw std::runtime_error("invalid runtime HIR call signature for '" + inst.callee + "'");
                        }
                        paramTypes = runtime->params;
                        returnType = runtime->ret;
                    } else {
                        auto calleeIt = std::find_if(module.functions.begin(), module.functions.end(),
                            [&](const Function& candidate) { return candidate.name == inst.callee; });
                        if (calleeIt == module.functions.end() || calleeIt->params.size() != inst.args.size()) {
                            throw std::runtime_error("invalid HIR call signature for '" + inst.callee + "'");
                        }
                        for (const auto& param : calleeIt->params) paramTypes.push_back(llvmType(param.second));
                    }
                    if (inst.type.kind != ScalarKind::Void) text += "  " + inst.result + " = ";
                    else text += "  ";
                    text += "call " + returnType + " @" + inst.callee + "(";
                    for (std::size_t i = 0; i < inst.args.size(); ++i) {
                        if (i) text += ", ";
                        text += paramTypes[i] + " " + inst.args[i];
                    }
                    text += ")\n";
                    break;
                }
                case Opcode::AddressOfLocal:
                    text += "  " + inst.result + " = getelementptr inbounds " + type + ", ptr %slot_" + inst.slot + ", i64 0\n";
                    break;
                case Opcode::AggregateIndex: {
                    const auto source = llvmType(inst.sourceType);
                    std::string index = inst.rhs;
                    if (inst.rhsType.kind == ScalarKind::Int && inst.rhsType.bits != 64) {
                        const std::string normalized = inst.result + ".index64";
                        const std::string op = inst.rhsType.isSigned ? "sext" : "zext";
                        text += "  " + normalized + " = " + op + " i" + std::to_string(inst.rhsType.bits) + " " + inst.rhs + " to i64\n";
                        index = normalized;
                    }
                    if (source == "%LannerDynArray") {
                        if (inst.sourceType.members.empty()) throw std::runtime_error("dynamic array HIR type has no element type");
                        const std::string array = inst.result + ".array";
                        const std::string data = inst.result + ".data";
                        text += "  " + array + " = load %LannerDynArray, ptr " + inst.lhs + "\n";
                        text += "  " + data + " = extractvalue %LannerDynArray " + array + ", 0\n";
                        text += "  " + inst.result + " = getelementptr inbounds " + llvmType(inst.sourceType.members[0]) + ", ptr " + data + ", i64 " + index + "\n";
                    } else if (inst.sourceType.sourceName == "View" || inst.sourceType.sourceName == "EditView") {
                        if (inst.sourceType.members.empty()) throw std::runtime_error("view HIR type has no element type");
                        const std::string view = inst.result + ".view";
                        const std::string data = inst.result + ".data";
                        if (inst.lhsType.llvmName == source) {
                            text += "  " + data + " = extractvalue " + source + " " + inst.lhs + ", 0\n";
                        } else {
                            text += "  " + view + " = load " + source + ", ptr " + inst.lhs + "\n";
                            text += "  " + data + " = extractvalue " + source + " " + view + ", 0\n";
                        }
                        text += "  " + inst.result + " = getelementptr inbounds " + llvmType(inst.sourceType.members[0]) + ", ptr " + data + ", i64 " + index + "\n";
                    } else if (!source.empty() && source.front() == '[') {
                        text += "  " + inst.result + " = getelementptr inbounds " + source + ", ptr " + inst.lhs + ", i64 0, i64 " + index + "\n";
                    } else {
                        if (!inst.aggregateIndexConstant) throw std::runtime_error("dynamic struct indexing is not supported");
                        text += "  " + inst.result + " = getelementptr inbounds " + source + ", ptr " + inst.lhs + ", i32 0, i32 " + std::to_string(inst.aggregateIndex) + "\n";
                    }
                    break;
                }
                case Opcode::LoadIndirect:
                    text += "  " + inst.result + " = load " + type + ", ptr " + inst.lhs + "\n";
                    break;
                case Opcode::StoreIndirect:
                    text += "  store " + type + " " + inst.rhs + ", ptr " + inst.lhs + "\n";
                    break;
                case Opcode::ExtractValue:
                    text += "  " + inst.result + " = extractvalue " + llvmType(inst.lhsType) + " " + inst.lhs + ", " + std::to_string(inst.aggregateIndex) + "\n";
                    break;
                case Opcode::InsertValue: {
                    const std::string base = inst.lhs.empty() ? "zeroinitializer" : inst.lhs;
                    text += "  " + inst.result + " = insertvalue " + type + " " + base + ", " + llvmType(inst.rhsType) + " " + inst.rhs + ", " + std::to_string(inst.aggregateIndex) + "\n";
                    break;
                }
                case Opcode::BoundsCheck: {
                    std::string index = inst.rhs;
                    if (inst.rhsType.kind != ScalarKind::Int) throw std::runtime_error("HIR bounds check requires integer index");
                    if (inst.rhsType.bits != 64) {
                        const std::string normalized = "%bounds_index64_" + std::to_string(internalCounter++);
                        const std::string op = inst.rhsType.isSigned ? "sext" : "zext";
                        text += "  " + normalized + " = " + op + " i" + std::to_string(inst.rhsType.bits) + " " + inst.rhs + " to i64\n";
                        index = normalized;
                    }
                    std::string limit;
                    if (inst.sourceType.llvmName == "%LannerDynArray") {
                        const std::string arr = "%bounds_arr_" + std::to_string(internalCounter++);
                        limit = "%bounds_len_" + std::to_string(internalCounter++);
                        text += "  " + arr + " = load %LannerDynArray, ptr " + inst.lhs + "\n";
                        text += "  " + limit + " = extractvalue %LannerDynArray " + arr + ", 1\n";
                    } else if (inst.sourceType.sourceName == "View" || inst.sourceType.sourceName == "EditView") {
                        const std::string view = "%bounds_view_" + std::to_string(internalCounter++);
                        limit = "%bounds_len_" + std::to_string(internalCounter++);
                        if (inst.lhsType.llvmName == llvmType(inst.sourceType)) {
                            text += "  " + limit + " = extractvalue " + llvmType(inst.sourceType) + " " + inst.lhs + ", 1\n";
                        } else {
                            text += "  " + view + " = load " + llvmType(inst.sourceType) + ", ptr " + inst.lhs + "\n";
                            text += "  " + limit + " = extractvalue " + llvmType(inst.sourceType) + " " + view + ", 1\n";
                        }
                    } else {
                        limit = std::to_string(inst.aggregateIndex);
                    }
                    const std::string ok = "hir.bounds.ok." + std::to_string(internalCounter++);
                    const std::string bad = "hir.bounds.bad." + std::to_string(internalCounter++);
                    const std::string cmp = "%bounds_ok_" + std::to_string(internalCounter++);
                    text += "  " + cmp + " = icmp ult i64 " + index + ", " + limit + "\n";
                    text += "  br i1 " + cmp + ", label %" + ok + ", label %" + bad + "\n";
                    text += bad + ":\n";
                    text += "  call void @llvm.trap()\n";
                    text += "  unreachable\n";
                    text += ok + ":\n";
                    currentFlowLabel = ok;
                    break;
                }
                case Opcode::DynamicArrayPush: {
                    if (inst.rhsType.kind == ScalarKind::Void) throw std::runtime_error("dynamic array push has void element type");
                    const std::string arrTmp = inst.result + ".push.array";
                    const std::string valueTmp = inst.result + ".push.value";
                    text += "  " + arrTmp + " = alloca %LannerDynArray, align 8\n";
                    text += "  store %LannerDynArray " + inst.lhs + ", ptr " + arrTmp + "\n";
                    text += "  " + valueTmp + " = alloca " + llvmType(inst.rhsType) + ", align 8\n";
                    text += "  store " + llvmType(inst.rhsType) + " " + inst.rhs + ", ptr " + valueTmp + "\n";
                    const std::string elemSize = "ptrtoint (ptr getelementptr (" + llvmType(inst.rhsType) + ", ptr null, i64 1) to i64)";
                    text += "  call void @__lanner_dyn_array_push(ptr " + arrTmp + ", ptr " + valueTmp + ", i64 " + elemSize + ")\n";
                    text += "  " + inst.result + " = load %LannerDynArray, ptr " + arrTmp + "\n";
                    break;
                }
                case Opcode::ArenaCreate: {
                    std::string size = inst.lhs;
                    if (inst.rhsType.kind == ScalarKind::Int && inst.rhsType.bits != 64) {
                        const std::string normalized = inst.result + ".arena.size";
                        const std::string op = inst.rhsType.isSigned ? "sext" : "zext";
                        text += "  " + normalized + " = " + op + " i" + std::to_string(inst.rhsType.bits) + " " + inst.lhs + " to i64\n";
                        size = normalized;
                    }
                    const std::string a0 = inst.result + ".arena.0";
                    const std::string a1 = inst.result + ".arena.1";
                    const std::string a2 = inst.result + ".arena.2";
                    text += "  " + a0 + " = insertvalue %LannerArena zeroinitializer, i64 " + size + ", 3\n";
                    text += "  " + a1 + " = insertvalue %LannerArena " + a0 + ", ptr null, 0\n";
                    text += "  " + a2 + " = insertvalue %LannerArena " + a1 + ", ptr null, 1\n";
                    text += "  " + inst.result + " = insertvalue %LannerArena " + a2 + ", i64 0, 2\n";
                    break;
                }
                case Opcode::DestroyLocal: {
                    currentFlowLabel = destroyAt(inst.type, "%slot_" + inst.slot, currentFlowLabel);
                    break;
                }
                case Opcode::DestroyAt: {
                    currentFlowLabel = destroyAt(inst.type, inst.lhs, currentFlowLabel);
                    break;
                }
                case Opcode::Branch:
                    text += "  br label %" + inst.targetLabel + "\n";
                    break;
                case Opcode::CondBranch:
                    text += "  br i1 " + inst.lhs + ", label %" + inst.trueLabel + ", label %" + inst.falseLabel + "\n";
                    break;
                case Opcode::Return:
                    if (inst.lhs.empty()) text += "  ret void\n";
                    else text += "  ret " + type + " " + inst.lhs + "\n";
                    break;
            }
        }
    };

    if (!fn.blocks.empty()) emitBlockInstructions(fn.blocks.front(), out);
    for (std::size_t i = 1; i < fn.blocks.size(); ++i) {
        out += fn.blocks[i].label + ":\n";
        emitBlockInstructions(fn.blocks[i], out);
    }
    out += "}\n\n";
}

std::string LLVMCodegen::generate(const Module& module) const {
    std::string out = "; ModuleID = 'lanner-hir'\n\n";
    out += "%LannerArena = type { ptr, ptr, i64, i64 }\n";
    out += "%LannerDynArray = type { ptr, i64, i64, ptr }\n";
    out += "%LannerArenaNode = type { ptr, i8 }\n\n";
    out += "declare void @llvm.trap()\n";
    out += "declare ptr @malloc(i64)\n";
    out += "declare ptr @realloc(ptr, i64)\n";
    out += "declare void @free(ptr)\n";
    out += "declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)\n";
    out += "declare { i64, i1 } @llvm.umul.with.overflow.i64(i64, i64)\n";
    out += "declare ptr @fopen(ptr, ptr)\n";
    out += "declare i32 @fseek(ptr, i64, i32)\n";
    out += "declare i64 @ftell(ptr)\n";
    out += "declare i64 @fread(ptr, i64, i64, ptr)\n";
    out += "declare i32 @fclose(ptr)\n";
    out += "declare i32 @puts(ptr)\n";
    out += "declare i64 @fwrite(ptr, i64, i64, ptr)\n";
    out += "declare i64 @strlen(ptr)\n";
    out += "declare ptr @getenv(ptr)\n";
    out += "declare i32 @printf(ptr, ...)\n\n";
    out += "@.lanner.rb = private unnamed_addr constant [3 x i8] c\"rb\\00\"\n";
    out += "@.lanner.print_i64 = private unnamed_addr constant [5 x i8] c\"%ld\\0A\\00\"\n";
    out += "@.lanner.print_u64 = private unnamed_addr constant [5 x i8] c\"%lu\\0A\\00\"\n";
    out += "@.lanner.print_f64 = private unnamed_addr constant [4 x i8] c\"%g\\0A\\00\"\n";
    out += "@.lanner.print_true = private unnamed_addr constant [5 x i8] c\"true\\00\"\n";
    out += "@.lanner.print_false = private unnamed_addr constant [6 x i8] c\"false\\00\"\n";
    out += "@.lanner.print_bool = private unnamed_addr constant [3 x i8] c\"%s\\00\"\n\n";
    out += "@.lanner.print_i64_raw = private unnamed_addr constant [4 x i8] c\"%ld\\00\"\n";
    out += "@stdout = external global ptr\n";
    out += "define internal ptr @__lanner_arena_alloc(ptr %arena, i64 %bytes) {\n";
    out += "entry:\n";
    out += "  %rem.addr = getelementptr inbounds %LannerArena, ptr %arena, i32 0, i32 2\n";
    out += "  %cur.addr = getelementptr inbounds %LannerArena, ptr %arena, i32 0, i32 1\n";
    out += "  %head.addr = getelementptr inbounds %LannerArena, ptr %arena, i32 0, i32 0\n";
    out += "  %chunk.addr = getelementptr inbounds %LannerArena, ptr %arena, i32 0, i32 3\n";
    out += "  %remaining = load i64, ptr %rem.addr\n";
    out += "  %current = load ptr, ptr %cur.addr\n";
    out += "  %align.sum = add i64 %bytes, 7\n";
    out += "  %align.ov = icmp ult i64 %align.sum, %bytes\n";
    out += "  br i1 %align.ov, label %oom, label %align.ok\n";
    out += "align.ok:\n";
    out += "  %alloc.bytes = and i64 %align.sum, -8\n";
    out += "  %has.current = icmp ne ptr %current, null\n";
    out += "  br i1 %has.current, label %check.space, label %new.chunk\n";
    out += "check.space:\n";
    out += "  %fits = icmp ule i64 %alloc.bytes, %remaining\n";
    out += "  br i1 %fits, label %bump, label %new.chunk\n";
    out += "bump:\n";
    out += "  %new.current = getelementptr i8, ptr %current, i64 %alloc.bytes\n";
    out += "  %new.remaining = sub i64 %remaining, %alloc.bytes\n";
    out += "  store ptr %new.current, ptr %cur.addr\n";
    out += "  store i64 %new.remaining, ptr %rem.addr\n";
    out += "  ret ptr %current\n";
    out += "new.chunk:\n";
    out += "  %chunk.size = load i64, ptr %chunk.addr\n";
    out += "  %chunk.zero = icmp eq i64 %chunk.size, 0\n";
    out += "  %base.capacity = select i1 %chunk.zero, i64 %alloc.bytes, i64 %chunk.size\n";
    out += "  %double.candidate = add i64 %base.capacity, %base.capacity\n";
    out += "  %double.wrapped = icmp ult i64 %double.candidate, %base.capacity\n";
    out += "  %grown.capacity = select i1 %double.wrapped, i64 %base.capacity, i64 %double.candidate\n";
    out += "  %need.bytes = icmp ult i64 %grown.capacity, %alloc.bytes\n";
    out += "  %capacity = select i1 %need.bytes, i64 %alloc.bytes, i64 %grown.capacity\n";
    out += "  %total = add i64 %capacity, 8\n";
    out += "  %total.ov = icmp ult i64 %total, %capacity\n";
    out += "  br i1 %total.ov, label %oom, label %allocate\n";
    out += "allocate:\n";
    out += "  %mem = call ptr @malloc(i64 %total)\n";
    out += "  %null = icmp eq ptr %mem, null\n";
    out += "  br i1 %null, label %oom, label %link\n";
    out += "link:\n";
    out += "  %head = load ptr, ptr %head.addr\n";
    out += "  store ptr %head, ptr %mem\n";
    out += "  store ptr %mem, ptr %head.addr\n";
    out += "  %payload = getelementptr inbounds %LannerArenaNode, ptr %mem, i32 0, i32 1\n";
    out += "  store ptr %payload, ptr %cur.addr\n";
    out += "  store i64 %capacity, ptr %rem.addr\n";
    out += "  store i64 %capacity, ptr %chunk.addr\n";
    out += "  br label %bump.after.chunk\n";
    out += "bump.after.chunk:\n";
    out += "  %fresh.current = load ptr, ptr %cur.addr\n";
    out += "  %fresh.remaining = load i64, ptr %rem.addr\n";
    out += "  %next.current = getelementptr i8, ptr %fresh.current, i64 %alloc.bytes\n";
    out += "  %next.remaining = sub i64 %fresh.remaining, %alloc.bytes\n";
    out += "  store ptr %next.current, ptr %cur.addr\n";
    out += "  store i64 %next.remaining, ptr %rem.addr\n";
    out += "  ret ptr %fresh.current\n";
    out += "oom:\n";
    out += "  call void @llvm.trap()\n";
    out += "  unreachable\n";
    out += "}\n\n";

    out += "define internal void @__lanner_arena_destroy(ptr %arena) {\n";
    out += "entry:\n";
    out += "  %first = load ptr, ptr %arena\n";
    out += "  br label %loop\n";
    out += "loop:\n";
    out += "  %node = phi ptr [ %first, %entry ], [ %next, %step ]\n";
    out += "  %has = icmp ne ptr %node, null\n";
    out += "  br i1 %has, label %free, label %done\n";
    out += "free:\n";
    out += "  %next = load ptr, ptr %node\n";
    out += "  call void @free(ptr %node)\n";
    out += "  br label %step\n";
    out += "step:\n";
    out += "  br label %loop\n";
    out += "done:\n";
    out += "  %head.addr = getelementptr inbounds %LannerArena, ptr %arena, i32 0, i32 0\n";
    out += "  %cur.addr = getelementptr inbounds %LannerArena, ptr %arena, i32 0, i32 1\n";
    out += "  %rem.addr = getelementptr inbounds %LannerArena, ptr %arena, i32 0, i32 2\n";
    out += "  store ptr null, ptr %head.addr\n";
    out += "  store ptr null, ptr %cur.addr\n";
    out += "  store i64 0, ptr %rem.addr\n";
    out += "  ret void\n";
    out += "}\n\n";

    out += "define internal void @__lanner_dyn_array_push(ptr %array, ptr %value, i64 %elem.size) {\n";
    out += "entry:\n";
    out += "  %data.addr = getelementptr inbounds %LannerDynArray, ptr %array, i32 0, i32 0\n";
    out += "  %len.addr = getelementptr inbounds %LannerDynArray, ptr %array, i32 0, i32 1\n";
    out += "  %cap.addr = getelementptr inbounds %LannerDynArray, ptr %array, i32 0, i32 2\n";
    out += "  %arena.addr = getelementptr inbounds %LannerDynArray, ptr %array, i32 0, i32 3\n";
    out += "  %data = load ptr, ptr %data.addr\n";
    out += "  %len = load i64, ptr %len.addr\n";
    out += "  %cap = load i64, ptr %cap.addr\n";
    out += "  %arena = load ptr, ptr %arena.addr\n";
    out += "  %full = icmp uge i64 %len, %cap\n";
    out += "  br i1 %full, label %grow, label %write\n";
    out += "grow:\n";
    out += "  %cap.nonzero = icmp ne i64 %cap, 0\n";
    out += "  br i1 %cap.nonzero, label %double.cap, label %initial.cap\n";
    out += "initial.cap:\n";
    out += "  br label %cap.merge\n";
    out += "double.cap:\n";
    out += "  %double = add i64 %cap, %cap\n";
    out += "  %double.wrap = icmp ult i64 %double, %cap\n";
    out += "  br i1 %double.wrap, label %oom, label %cap.ok\n";
    out += "cap.ok:\n";
    out += "  br label %cap.merge\n";
    out += "cap.merge:\n";
    out += "  %new.cap = phi i64 [ 4, %initial.cap ], [ %double, %cap.ok ]\n";
    out += "  %bytes.pair = call { i64, i1 } @llvm.umul.with.overflow.i64(i64 %new.cap, i64 %elem.size)\n";
    out += "  %bytes = extractvalue { i64, i1 } %bytes.pair, 0\n";
    out += "  %bytes.ov = extractvalue { i64, i1 } %bytes.pair, 1\n";
    out += "  br i1 %bytes.ov, label %oom, label %alloc.mode\n";
    out += "alloc.mode:\n";
    out += "  %has.arena = icmp ne ptr %arena, null\n";
    out += "  br i1 %has.arena, label %arena.alloc, label %heap.alloc\n";
    out += "arena.alloc:\n";
    out += "  %arena.data = call ptr @__lanner_arena_alloc(ptr %arena, i64 %bytes)\n";
    out += "  br label %alloc.merge\n";
    out += "heap.alloc:\n";
    out += "  %heap.data = call ptr @realloc(ptr %data, i64 %bytes)\n";
    out += "  %heap.null = icmp eq ptr %heap.data, null\n";
    out += "  br i1 %heap.null, label %oom, label %heap.ok\n";
    out += "heap.ok:\n";
    out += "  br label %alloc.merge\n";
    out += "alloc.merge:\n";
    out += "  %new.data = phi ptr [ %arena.data, %arena.alloc ], [ %heap.data, %heap.ok ]\n";
    out += "  %old.nonnull = icmp ne ptr %data, null\n";
    out += "  br i1 %old.nonnull, label %maybe.copy, label %copy.done\n";
    out += "maybe.copy:\n";
    out += "  br i1 %has.arena, label %copy.arena, label %copy.heap\n";
    out += "copy.arena:\n";
    out += "  %old.bytes.pair = call { i64, i1 } @llvm.umul.with.overflow.i64(i64 %len, i64 %elem.size)\n";
    out += "  %old.bytes = extractvalue { i64, i1 } %old.bytes.pair, 0\n";
    out += "  %old.bytes.ov = extractvalue { i64, i1 } %old.bytes.pair, 1\n";
    out += "  br i1 %old.bytes.ov, label %oom, label %copy.arena.do\n";
    out += "copy.arena.do:\n";
    out += "  call void @llvm.memcpy.p0.p0.i64(ptr %new.data, ptr %data, i64 %old.bytes, i1 false)\n";
    out += "  br label %copy.done\n";
    out += "copy.heap:\n";
    out += "  br label %copy.done\n";
    out += "copy.done:\n";
    out += "  store ptr %new.data, ptr %data.addr\n";
    out += "  store i64 %new.cap, ptr %cap.addr\n";
    out += "  br label %write\n";
    out += "write:\n";
    out += "  %data.now = load ptr, ptr %data.addr\n";
    out += "  %len.now = load i64, ptr %len.addr\n";
    out += "  %offset.pair = call { i64, i1 } @llvm.umul.with.overflow.i64(i64 %len.now, i64 %elem.size)\n";
    out += "  %offset = extractvalue { i64, i1 } %offset.pair, 0\n";
    out += "  %offset.ov = extractvalue { i64, i1 } %offset.pair, 1\n";
    out += "  br i1 %offset.ov, label %oom, label %store\n";
    out += "store:\n";
    out += "  %elem.addr = getelementptr i8, ptr %data.now, i64 %offset\n";
    out += "  call void @llvm.memcpy.p0.p0.i64(ptr %elem.addr, ptr %value, i64 %elem.size, i1 false)\n";
    out += "  %next.len = add i64 %len.now, 1\n";
    out += "  store i64 %next.len, ptr %len.addr\n";
    out += "  ret void\n";
    out += "oom:\n";
    out += "  call void @llvm.trap()\n";
    out += "  unreachable\n";
    out += "}\n\n";

    out += "define internal %LannerDynArray @__lanner_read_file(ptr %path) {\n";
    out += "entry:\n";
    out += "  %file = call ptr @fopen(ptr %path, ptr @.lanner.rb)\n";
    out += "  %missing = icmp eq ptr %file, null\n";
    out += "  br i1 %missing, label %fail.no.close, label %seek.end\n";
    out += "seek.end:\n";
    out += "  %seek.rc = call i32 @fseek(ptr %file, i64 0, i32 2)\n";
    out += "  %seek.bad = icmp ne i32 %seek.rc, 0\n";
    out += "  br i1 %seek.bad, label %close.fail, label %size\n";
    out += "size:\n";
    out += "  %file.size = call i64 @ftell(ptr %file)\n";
    out += "  %size.bad = icmp slt i64 %file.size, 0\n";
    out += "  br i1 %size.bad, label %close.fail, label %rewind\n";
    out += "rewind:\n";
    out += "  %rewind.rc = call i32 @fseek(ptr %file, i64 0, i32 0)\n";
    out += "  %rewind.bad = icmp ne i32 %rewind.rc, 0\n";
    out += "  br i1 %rewind.bad, label %close.fail, label %alloc\n";
    out += "alloc:\n";
    out += "  %plus.one = add i64 %file.size, 1\n";
    out += "  %plus.overflow = icmp ult i64 %plus.one, %file.size\n";
    out += "  br i1 %plus.overflow, label %close.fail, label %malloc.data\n";
    out += "malloc.data:\n";
    out += "  %data = call ptr @malloc(i64 %plus.one)\n";
    out += "  %alloc.bad = icmp eq ptr %data, null\n";
    out += "  br i1 %alloc.bad, label %close.fail, label %read\n";
    out += "read:\n";
    out += "  %count = call i64 @fread(ptr %data, i64 1, i64 %file.size, ptr %file)\n";
    out += "  %read.bad = icmp ne i64 %count, %file.size\n";
    out += "  br i1 %read.bad, label %free.fail, label %finish\n";
    out += "finish:\n";
    out += "  %end = getelementptr inbounds i8, ptr %data, i64 %file.size\n";
    out += "  store i8 0, ptr %end\n";
    out += "  call i32 @fclose(ptr %file)\n";
    out += "  %a0 = insertvalue %LannerDynArray zeroinitializer, ptr %data, 0\n";
    out += "  %a1 = insertvalue %LannerDynArray %a0, i64 %file.size, 1\n";
    out += "  %a2 = insertvalue %LannerDynArray %a1, i64 %file.size, 2\n";
    out += "  %a3 = insertvalue %LannerDynArray %a2, ptr null, 3\n";
    out += "  ret %LannerDynArray %a3\n";
    out += "free.fail:\n";
    out += "  call void @free(ptr %data)\n";
    out += "  br label %close.fail\n";
    out += "close.fail:\n";
    out += "  call i32 @fclose(ptr %file)\n";
    out += "  br label %fail\n";
    out += "fail.no.close:\n";
    out += "  br label %fail\n";
    out += "fail:\n";
    out += "  call void @llvm.trap()\n";
    out += "  unreachable\n";
    out += "}\n\n";

    out += "@.lanner.empty = private unnamed_addr constant [1 x i8] c\"\\00\"\n\n";
    out += "define internal ptr @__lanner_getenv(ptr %name) {\n";
    out += "entry:\n";
    out += "  %value = call ptr @getenv(ptr %name)\n";
    out += "  %missing = icmp eq ptr %value, null\n";
    out += "  br i1 %missing, label %empty, label %found\n";
    out += "empty:\n";
    out += "  ret ptr getelementptr inbounds ([1 x i8], ptr @.lanner.empty, i64 0, i64 0)\n";
    out += "found:\n";
    out += "  ret ptr %value\n";
    out += "}\n\n";

    out += "define internal void @__lanner_print_i64(i64 %value) {\n";
    out += "entry:\n";
    out += "  call i32 (ptr, ...) @printf(ptr @.lanner.print_i64, i64 %value)\n";
    out += "  ret void\n";
    out += "}\n\n";
    out += "define internal void @__lanner_print_u64(i64 %value) {\n";
    out += "entry:\n";
    out += "  call i32 (ptr, ...) @printf(ptr @.lanner.print_u64, i64 %value)\n";
    out += "  ret void\n";
    out += "}\n\n";
    out += "define internal void @__lanner_print_f64(double %value) {\n";
    out += "entry:\n";
    out += "  call i32 (ptr, ...) @printf(ptr @.lanner.print_f64, double %value)\n";
    out += "  ret void\n";
    out += "}\n\n";
    out += "define internal void @__lanner_print_bool(i1 %value) {\n";
    out += "entry:\n";
    out += "  %ptr = select i1 %value, ptr getelementptr inbounds ([5 x i8], ptr @.lanner.print_true, i64 0, i64 0), ptr getelementptr inbounds ([6 x i8], ptr @.lanner.print_false, i64 0, i64 0)\n";
    out += "  call i32 @puts(ptr %ptr)\n";
    out += "  ret void\n";
    out += "}\n\n";
    out += "define internal void @__lanner_print_string(ptr %value) {\n";
    out += "entry:\n";
    out += "  call i32 @puts(ptr %value)\n";
    out += "  ret void\n";
    out += "}\n\n";
    out += "define internal void @__lanner_write_stdout(ptr %value) {\n";
    out += "entry:\n";
    out += "  call i32 @puts(ptr %value)\n";
    out += "  ret void\n";
    out += "}\n\n";
    out += "define internal void @__lanner_write_raw(ptr %value) {\n";
    out += "entry:\n";
    out += "  %len = call i64 @strlen(ptr %value)\n";
    out += "  %out = load ptr, ptr @stdout\n";
    out += "  call i64 @fwrite(ptr %value, i64 1, i64 %len, ptr %out)\n";
    out += "  ret void\n";
    out += "}\n\n";
    out += "define internal void @__lanner_write_i64_raw(i64 %value) {\n";
    out += "entry:\n";
    out += "  call i32 (ptr, ...) @printf(ptr @.lanner.print_i64_raw, i64 %value)\n";
    out += "  ret void\n";
    out += "}\n\n";
    out += "define internal void @__lanner_write_byte_raw(i64 %value) {\n";
    out += "entry:\n";
    out += "  %byte = trunc i64 %value to i8\n";
    out += "  %buf = alloca i8, align 1\n";
    out += "  store i8 %byte, ptr %buf\n";
    out += "  %out = load ptr, ptr @stdout\n";
    out += "  call i64 @fwrite(ptr %buf, i64 1, i64 1, ptr %out)\n";
    out += "  ret void\n";
    out += "}\n\n";
    out += "define internal i64 @__lanner_string_len(ptr %value) {\n";
    out += "entry:\n";
    out += "  %len = call i64 @strlen(ptr %value)\n";
    out += "  ret i64 %len\n";
    out += "}\n\n";

    for (const auto& fn : module.functions) {
        for (const auto& block : fn.blocks) {
            for (const auto& inst : block.instructions) {
                if (inst.op != Opcode::ConstString) continue;
                std::string encoded;
                encoded.reserve(inst.stringValue.size() * 3 + 3);
                const auto hex = [](unsigned v) -> std::string {
                    const char digits[] = "0123456789ABCDEF";
                    std::string r; r.push_back(digits[(v >> 4) & 0xF]); r.push_back(digits[v & 0xF]); return r;
                };
                for (unsigned char c : inst.stringValue) {
                    if (c >= 0x20 && c <= 0x7E && c != '"' && c != '\\') encoded.push_back(static_cast<char>(c));
                    else { encoded += "\\"; encoded += hex(c); }
                }
                encoded += "\\00";
                const std::string global = "@.lanner.str." + fn.name + "." + (inst.result.empty() ? "anon" : inst.result.substr(1));
                out += global + " = private unnamed_addr constant [" + std::to_string(inst.stringValue.size() + 1) + " x i8] c\"" + encoded + "\"\n";
            }
        }
    }
    bool hasStringConstants = false;
    for (const auto& fn : module.functions) for (const auto& block : fn.blocks) for (const auto& inst : block.instructions) if (inst.op == Opcode::ConstString) hasStringConstants = true;
    if (hasStringConstants) out += "\n";

    for (const auto& aggregate : module.aggregates) {
        out += aggregate.name + " = type " + aggregate.body + "\n";
    }
    if (!module.aggregates.empty()) out += "\n";
    for (const auto& fn : module.functions) emitFunction(module, fn, out);
    return out;
}

} // namespace lanner::hir
