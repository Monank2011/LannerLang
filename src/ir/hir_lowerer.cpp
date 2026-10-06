#include "hir_lowerer.hpp"
#include "../sema/typechecker.hpp"
#include "../comptime/comptime_eval.hpp"
#include "../sema/symbol_table.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

namespace lanner::hir {

namespace {

std::uint64_t parseUInt(const std::string& text) {
    int base = 10;
    std::size_t start = 0;
    if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        base = 16; start = 2;
    } else if (text.size() > 2 && text[0] == '0' && (text[1] == 'b' || text[1] == 'B')) {
        base = 2; start = 2;
    }
    std::uint64_t value = 0;
    const char* first = text.data() + start;
    const char* last = text.data() + text.size();
    auto [ptr, ec] = std::from_chars(first, last, value, base);
    if (ec != std::errc{} || ptr != last) throw std::runtime_error("invalid integer literal");
    return value;
}

bool isCompareOp(const std::string& op) {
    return op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=";
}

MemoryKind memoryKindOf(const TypeNode* type) {
    if (!type) return MemoryKind::Owned;
    if (type->isReference) return type->isMutable ? MemoryKind::ExclusiveReference : MemoryKind::SharedReference;
    if (type->name == "EditView") return MemoryKind::ExclusiveView;
    if (type->name == "View") return MemoryKind::SharedView;
    return MemoryKind::Owned;
}

Type annotateMemory(Type value, const TypeNode* source) {
    if (!source) return value;
    value.memory = memoryKindOf(source);
    value.origin = source->origin;
    value.nestedOrigins = source->nestedOrigins;
    return value;
}

std::unique_ptr<TypeNode> cloneTypeNode(const TypeNode* type) {
    if (!type) return nullptr;
    auto copy = std::make_unique<TypeNode>();
    copy->name = type->name;
    copy->line = type->line;
    copy->column = type->column;
    copy->isArray = type->isArray;
    copy->fixedArraySize = type->fixedArraySize;
    copy->isOptional = type->isOptional;
    copy->isReference = type->isReference;
    copy->isMutable = type->isMutable;
    copy->origin = type->origin;
    copy->nestedOrigins = type->nestedOrigins;
    for (const auto& generic : type->generics) copy->generics.push_back(cloneTypeNode(generic.get()));
    return copy;
}

std::unique_ptr<TypeNode> arrayElementType(const TypeNode* arrayType) {
    auto element = cloneTypeNode(arrayType);
    element->isArray = false;
    element->fixedArraySize.reset();
    element->origin = {};
    element->nestedOrigins.clear();
    element->isReference = false;
    element->isMutable = false;
    return element;
}

} // namespace

void Lowerer::registerProgram(const Program& p) {
    functions.clear();
    enums.clear();
    structs.clear();
    comptimeGlobals.clear();
    comptimeScopes.clear();
    SymbolTable comptimeSymbols;
    ComptimeEvaluator evaluator(comptimeSymbols);
    for (const auto& d : p.decls) {
        if (d->kind == DeclKind::Function && d->fn) functions[d->fn->name] = d->fn.get();
        if (d->kind == DeclKind::Enum && d->en) enums[d->en->name] = d->en.get();
        if (d->kind == DeclKind::Struct && d->st) structs[d->st->name] = d->st.get();
        if (d->kind == DeclKind::ComptimeGlobal && d->comptimeGlobal) {
            evaluator.evaluateAndDeclare(d->comptimeGlobal.get());
            if (const auto* sym = comptimeSymbols.resolve(d->comptimeGlobal->comptimeName);
                sym && sym->comptimeValue.has_value()) {
                comptimeGlobals[d->comptimeGlobal->comptimeName] = *sym->comptimeValue;
            }
        }
    }
}

Module Lowerer::lowerProgram(const Program& p) {
    program = &p;
    registerProgram(p);
    Module m;

    for (const auto& d : p.decls) {
        if (d->kind != DeclKind::Struct || !d->st) continue;
        std::string body = "{ ";
        for (std::size_t i = 0; i < d->st->fields.size(); ++i) {
            if (i) body += ", ";
            body += lowerType(d->st->fields[i].type.get()).llvmName;
        }
        body += " }";
        m.aggregates.push_back(AggregateDecl{"%" + d->st->name, body});
    }

    for (const auto& d : p.decls) {
        if (d->kind != DeclKind::Function || !d->fn) continue;
        m.functions.push_back(lowerFunction(*d->fn));
    }
    return m;
}

Type Lowerer::lowerType(const TypeNode* type) const {
    if (!type) throw std::runtime_error("HIR requires a typechecked type");

    if (type->isReference) {
        return annotateMemory({ScalarKind::Aggregate, 0, false,
                               "&" + std::string(type->isMutable ? "mut " : "") + type->name,
                               "ptr", {}, 0}, type);
    }

    if (type->isOptional) {
        auto payload = cloneTypeNode(type);
        payload->isOptional = false;
        const Type payloadType = lowerType(payload.get());
        return annotateMemory(Type{ScalarKind::Aggregate, 0, false, type->name + "?",
                                   "{ i1, " + payloadType.llvmName + " }", {payloadType}, 0}, type);
    }

    if (type->name == "Result") {
        if (type->generics.size() != 2) {
            throw std::runtime_error("HIR requires Result[Ok, Err] to have two type arguments");
        }
        const Type okType = lowerType(type->generics[0].get());
        const Type errType = lowerType(type->generics[1].get());
        return annotateMemory(Type{ScalarKind::Aggregate, 0, false,
                                   "Result", "{ i1, " + okType.llvmName + ", " + errType.llvmName + " }",
                                   {okType, errType}, 0}, type);
    }

    if (type->name == "Arena") {
        return annotateMemory({ScalarKind::Aggregate, 0, false, "Arena", "%LannerArena", {}, 0}, type);
    }

    if ((type->name == "View" || type->name == "EditView") && type->generics.size() == 1) {
        const Type elementType = lowerType(type->generics[0].get());
        return annotateMemory({ScalarKind::Aggregate, 0, false, type->name, "{ ptr, i64 }", {elementType}, 0}, type);
    }

    if (type->isArray) {
        auto element = arrayElementType(type);
        const Type elementType = lowerType(element.get());
        if (type->fixedArraySize) {
            return annotateMemory({ScalarKind::Aggregate, 0, false, "array", "[" + std::to_string(*type->fixedArraySize) +
                    " x " + elementType.llvmName + "]", {elementType}, *type->fixedArraySize}, type);
        }
        return annotateMemory({ScalarKind::Aggregate, 0, false, "array", "%LannerDynArray", {elementType}, 0}, type);
    }

    if (type->name == "string") {
        return annotateMemory({ScalarKind::String, 64, false, type->name, "ptr"}, type);
    }

    if (structs.count(type->name)) {
        std::vector<Type> members;
        members.reserve(structs.at(type->name)->fields.size());
        for (const auto& field : structs.at(type->name)->fields) members.push_back(lowerType(field.type.get()));
        return annotateMemory({ScalarKind::Aggregate, 0, false, type->name, "%" + type->name, std::move(members), 0}, type);
    }

    return annotateMemory(lowerScalarType(type, enums), type);
}

Type Lowerer::declaredType(const TypeNode* type) const { return lowerType(type); }

Type Lowerer::exprType(const Expr* expr) const {
    if (!expr || !expr->checkedType) throw std::runtime_error("HIR requires typechecked expressions");
    return declaredType(expr->checkedType.get());
}

Function Lowerer::lowerFunction(const FunctionDecl& fn) {
    current = {};
    current.name = fn.name;
    current.returnType = declaredType(fn.returnType.get());
    current.params.reserve(fn.params.size());
    for (const auto& p : fn.params) {
        Type t = declaredType(p.type.get());
        current.params.emplace_back(p.name, t);
        current.locals.emplace(p.name, t);
    }

    tempCounter = 0;
    blockCounter = 0;
    localScopes.clear();
    scopeOrder.clear();
    breakTargets.clear();
    continueTargets.clear();
    loopScopeDepths.clear();
    initializedSlots.clear();
    movedSlots.clear();
    destroyedSlots.clear();
    current.blocks.push_back(BasicBlock{"entry", {}});
    currentBlock = 0;
    pushScope();
    for (const auto& p : fn.params) {
        localScopes.back()[p.name] = p.name;
        scopeOrder.back().push_back(p.name);
        initializedSlots.insert(p.name);
    }

    lowerBlock(fn.body);
    if (!terminated()) {
        cleanupScopes();
        if (current.returnType.kind == ScalarKind::Void) {
            Instruction ret;
            ret.op = Opcode::Return;
            emit(std::move(ret));
        } else {
            unsupported("non-void function may fall through", fn.line);
        }
    }
    popScope();
    return current;
}

BasicBlock& Lowerer::block() { return current.blocks[currentBlock]; }
bool Lowerer::terminated() const {
    if (current.blocks.empty()) return true;
    if (current.blocks[currentBlock].instructions.empty()) return false;
    const auto op = current.blocks[currentBlock].instructions.back().op;
    if (op == Opcode::Call && !current.blocks[currentBlock].instructions.back().callee.empty() &&
        current.blocks[currentBlock].instructions.back().callee == "llvm.trap") return true;
    return op == Opcode::Branch || op == Opcode::CondBranch || op == Opcode::Return;
}

void Lowerer::emit(const Instruction& inst) { block().instructions.push_back(inst); }

void Lowerer::ensureBranchTo(const std::string& label) {
    if (terminated()) return;
    Instruction br;
    br.op = Opcode::Branch;
    br.targetLabel = label;
    emit(std::move(br));
}

std::string Lowerer::newTemp() { return "%t" + std::to_string(++tempCounter); }

std::string Lowerer::newBlockLabel(const std::string& prefix) {
    return prefix + std::to_string(++blockCounter);
}

void Lowerer::pushScope() {
    localScopes.emplace_back();
    scopeOrder.emplace_back();
    comptimeScopes.emplace_back();
}
void Lowerer::popScope() {
    if (!localScopes.empty()) localScopes.pop_back();
    if (!scopeOrder.empty()) scopeOrder.pop_back();
    if (!comptimeScopes.empty()) comptimeScopes.pop_back();
}

const ComptimeValue* Lowerer::lookupComptime(const std::string& name) const {
    for (auto it = comptimeScopes.rbegin(); it != comptimeScopes.rend(); ++it) {
        const auto found = it->find(name);
        if (found != it->end()) return &found->second;
    }
    const auto global = comptimeGlobals.find(name);
    return global == comptimeGlobals.end() ? nullptr : &global->second;
}

std::string Lowerer::declareLocal(const std::string& sourceName) {
    auto& scope = localScopes.back();
    auto it = scope.find(sourceName);
    if (it != scope.end()) return it->second;
    const std::string slot = sourceName + "." + std::to_string(tempCounter + static_cast<int>(current.locals.size()) + 1);
    scope[sourceName] = slot;
    scopeOrder.back().push_back(slot);
    return slot;
}

std::string Lowerer::lookupCurrentLocal(const std::string& sourceName) const {
    if (localScopes.empty()) return {};
    auto it = localScopes.back().find(sourceName);
    return it == localScopes.back().end() ? std::string{} : it->second;
}

std::string Lowerer::lookupLocal(const std::string& sourceName) const {
    for (auto it = localScopes.rbegin(); it != localScopes.rend(); ++it) {
        auto found = it->find(sourceName);
        if (found != it->end()) return found->second;
    }
    return {};
}

void Lowerer::lowerBlock(const std::vector<std::unique_ptr<Stmt>>& stmts) {
    for (const auto& stmt : stmts) {
        if (terminated()) break;
        lowerStmt(stmt.get());
    }
}

std::string Lowerer::addressOfLocal(const std::string& sourceName, int line) {
    const auto slot = lookupLocal(sourceName);
    if (slot.empty()) unsupported("unknown local '" + sourceName + "'", line);
    const auto localIt = current.locals.find(slot);
    if (localIt == current.locals.end()) unsupported("unknown HIR local slot '" + slot + "'", line);
    Instruction addr;
    addr.op = Opcode::AddressOfLocal;
    addr.type = localIt->second;
    addr.result = newTemp();
    addr.slot = slot;
    emit(std::move(addr));
    return block().instructions.back().result;
}

std::size_t Lowerer::structFieldIndex(const TypeNode* type, const std::string& field) const {
    if (!type) unsupported("missing struct type", 0);
    auto normalized = cloneTypeNode(type);
    if (normalized->isReference) normalized->isReference = false;
    auto it = structs.find(normalized->name);
    if (it == structs.end()) unsupported("unknown struct '" + normalized->name + "'", normalized->line);
    for (std::size_t i = 0; i < it->second->fields.size(); ++i) {
        if (it->second->fields[i].name == field) return i;
    }
    unsupported("unknown struct field '" + field + "'", type->line);
}

std::string Lowerer::lowerFieldEnum(const Expr* expr) {
    if (!expr || expr->kind != ExprKind::FieldAccess || !expr->target || expr->target->kind != ExprKind::Identifier) {
        unsupported("unsupported enum field expression", expr ? expr->line : 0);
    }
    const auto& enumName = expr->target->strValue;
    auto it = enums.find(enumName);
    if (it == enums.end()) unsupported("unknown enum '" + enumName + "'", expr->line);
    for (const auto& v : it->second->variants) {
        if (v.name != expr->field) continue;
        Instruction c;
        c.op = Opcode::ConstInt;
        c.type = exprType(expr);
        c.result = newTemp();
        c.intValue = v.value;
        emit(std::move(c));
        return block().instructions.back().result;
    }
    unsupported("unknown enum variant '" + enumName + "." + expr->field + "'", expr->line);
}

std::string Lowerer::lowerStructFieldValue(const Expr* expr) {
    const auto base = lowerExpr(expr->target.get());
    const auto index = structFieldIndex(expr->target->checkedType.get(), expr->field);
    Instruction extract;
    extract.op = Opcode::ExtractValue;
    extract.type = exprType(expr);
    extract.result = newTemp();
    extract.lhs = base;
    extract.lhsType = exprType(expr->target.get());
    extract.aggregateIndex = index;
    emit(std::move(extract));
    return block().instructions.back().result;
}

std::string Lowerer::lowerAggregateIndexAddress(const Expr* expr) {
    const auto* targetType = expr && expr->target ? expr->target->checkedType.get() : nullptr;
    if (!targetType) unsupported("indexed target has no type", expr ? expr->line : 0);
    auto normalized = cloneTypeNode(targetType);
    if (normalized->isReference) normalized->isReference = false;
    if (!(normalized->isArray || normalized->name == "View" || normalized->name == "EditView")) {
        unsupported("HIR indexing requires an array, View, or EditView", expr->line);
    }

    std::string base;
    if (targetType->isReference || normalized->name == "View" || normalized->name == "EditView") {
        base = lowerExpr(expr->target.get());
    } else {
        base = lowerLValueAddress(expr->target.get());
    }

    const auto index = lowerExpr(expr->args.at(0).get());
    Instruction bounds;
    bounds.op = Opcode::BoundsCheck;
    bounds.type = Type{ScalarKind::Bool, 1, false, "bool", "i1"};
    bounds.rhs = index;
    bounds.rhsType = exprType(expr->args[0].get());
    bounds.lhs = base;
    bounds.lhsType = exprType(expr->target.get());
    bounds.sourceType = lowerType(normalized.get());
    if (normalized->isArray && normalized->fixedArraySize) {
        bounds.aggregateIndex = *normalized->fixedArraySize;
        bounds.aggregateIndexConstant = true;
    }
    emit(std::move(bounds));

    Instruction addr;
    addr.op = Opcode::AggregateIndex;
    addr.type = lowerType(normalized->isArray
        ? arrayElementType(normalized.get()).get()
        : normalized->generics.front().get());
    addr.result = newTemp();
    addr.lhs = base;
    addr.lhsType = exprType(expr->target.get());
    addr.rhs = index;
    addr.rhsType = exprType(expr->args[0].get());
    addr.sourceType = lowerType(normalized.get());
    addr.aggregateIndexConstant = normalized->isArray && normalized->fixedArraySize.has_value();
    if (!addr.aggregateIndexConstant) addr.aggregateIndex = 0;
    emit(std::move(addr));
    return block().instructions.back().result;
}

std::string Lowerer::lowerLValueAddress(const Expr* expr) {
    if (!expr) unsupported("null lvalue", 0);
    if (expr->kind == ExprKind::Identifier) {
        const auto slot = lookupLocal(expr->strValue);
        if (slot.empty()) unsupported("unknown local '" + expr->strValue + "'", expr->line);
        const auto localIt = current.locals.find(slot);
        if (localIt == current.locals.end()) unsupported("unknown HIR local slot '" + slot + "'", expr->line);
        const auto bindingAddr = addressOfLocal(expr->strValue, expr->line);
        if (localIt->second.memory == MemoryKind::ExclusiveReference || localIt->second.memory == MemoryKind::SharedReference) {
            Instruction load;
            load.op = Opcode::LoadIndirect;
            load.type = Type{ScalarKind::Aggregate, 0, false, "&ref-pointee", "ptr"};
            load.result = newTemp();
            load.lhs = bindingAddr;
            emit(std::move(load));
            return block().instructions.back().result;
        }
        return bindingAddr;
    }
    if (expr->kind == ExprKind::FieldAccess) {
        const auto* targetType = expr->target->checkedType.get();
        if (!targetType) unsupported("field target has no type", expr->line);
        if (targetType->isReference) {
            const auto object = lowerExpr(expr->target.get());
            const auto idx = structFieldIndex(targetType, expr->field);
            auto valueType = cloneTypeNode(targetType);
            valueType->isReference = false;
            Instruction addr;
            addr.op = Opcode::AggregateIndex;
            addr.type = lowerType(valueType.get());
            addr.result = newTemp();
            addr.lhs = object;
            addr.sourceType = lowerType(valueType.get());
            addr.aggregateIndex = idx;
            addr.aggregateIndexConstant = true;
            emit(std::move(addr));
            return block().instructions.back().result;
        }
        const auto base = lowerLValueAddress(expr->target.get());
        const auto idx = structFieldIndex(targetType, expr->field);
        Instruction addr;
        addr.op = Opcode::AggregateIndex;
        addr.type = exprType(expr);
        addr.result = newTemp();
        addr.lhs = base;
        addr.sourceType = lowerType(targetType);
        addr.aggregateIndex = idx;
        addr.aggregateIndexConstant = true;
        emit(std::move(addr));
        return block().instructions.back().result;
    }
    if (expr->kind == ExprKind::Index) return lowerAggregateIndexAddress(expr);
    unsupported("unsupported lvalue in HIR", expr->line);
}

void Lowerer::markMovedIfOwned(const Expr* expr) {
    if (!expr || expr->kind != ExprKind::Identifier || !expr->checkedType || TypeChecker::isCopyType(expr->checkedType.get())) return;
    const auto slot = lookupLocal(expr->strValue);
    if (!slot.empty()) movedSlots.insert(slot);
}

void Lowerer::emitStoreLocal(const std::string& slot, const Type& type, const std::string& value, const Expr* rhs) {
    if (initializedSlots.count(slot) && !movedSlots.count(slot)) {
        cleanupLocal(slot, current.locals.at(slot));
    }
    Instruction store;
    store.op = Opcode::StoreLocal;
    store.type = type;
    store.slot = slot;
    store.lhs = value;
    emit(std::move(store));
    initializedSlots.insert(slot);
    destroyedSlots.erase(slot);
    if (rhs) markMovedIfOwned(rhs);
}

void Lowerer::cleanupLocal(const std::string& slot, const Type& type) {
    if (!initializedSlots.count(slot) || movedSlots.count(slot) || destroyedSlots.count(slot)) return;
    Instruction drop;
    drop.op = Opcode::DestroyLocal;
    drop.type = type;
    drop.slot = slot;
    emit(std::move(drop));
    destroyedSlots.insert(slot);
}

void Lowerer::emitDestroyAt(const std::string& address, const Type& type) {
    const bool owning = type.llvmName == "%LannerDynArray" || type.llvmName == "%LannerArena" ||
        (!type.members.empty() && type.sourceName != "View" && type.sourceName != "EditView" &&
         (type.sourceName == "Result" || type.sourceName.back() == '?' || type.sourceName == "array" ||
          type.sourceName == "struct" || type.sourceName.find('%') == std::string::npos));
    if (!owning) return;
    Instruction drop;
    drop.op = Opcode::DestroyAt;
    drop.type = type;
    drop.lhs = address;
    emit(std::move(drop));
}

void Lowerer::cleanupCurrentScope() {
    if (scopeOrder.empty()) return;
    const auto& order = scopeOrder.back();
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        const auto typeIt = current.locals.find(*it);
        if (typeIt != current.locals.end()) cleanupLocal(*it, typeIt->second);
    }
}

void Lowerer::cleanupScopesFrom(std::size_t firstDepth) {
    if (firstDepth >= scopeOrder.size()) return;
    for (std::size_t depth = scopeOrder.size(); depth > firstDepth; --depth) {
        const auto& order = scopeOrder[depth - 1];
        for (auto it = order.rbegin(); it != order.rend(); ++it) {
            const auto typeIt = current.locals.find(*it);
            if (typeIt != current.locals.end()) cleanupLocal(*it, typeIt->second);
        }
    }
}

void Lowerer::cleanupScopes() {
    cleanupScopesFrom(0);
}

std::string Lowerer::lowerCall(const Expr* expr) {
    if (expr->callee->kind == ExprKind::FieldAccess) {
        const auto* field = expr->callee.get();
        const auto* targetType = field->target->checkedType.get();
        if (field->target->kind == ExprKind::Identifier && field->target->strValue == "Mobile") {
            static const std::unordered_map<std::string, std::string> mobileCalls = {
                {"log", "__lanner_mobile_log"},
                {"platform", "__lanner_mobile_platform"},
                {"osVersion", "__lanner_mobile_os_version"},
                {"isSimulator", "__lanner_mobile_is_simulator"},
                {"screenWidth", "__lanner_mobile_screen_width"},
                {"screenHeight", "__lanner_mobile_screen_height"},
                {"deviceScale", "__lanner_mobile_device_scale"},
                {"safeAreaTop", "__lanner_mobile_safe_top"},
                {"safeAreaBottom", "__lanner_mobile_safe_bottom"},
                {"safeAreaLeft", "__lanner_mobile_safe_left"},
                {"safeAreaRight", "__lanner_mobile_safe_right"},
                {"openUrl", "__lanner_mobile_open_url"},
                {"vibrate", "__lanner_mobile_vibrate"},
                {"requestPermission", "__lanner_mobile_request_permission"},
                {"clipboardSet", "__lanner_mobile_clipboard_set"},
                {"clipboardGet", "__lanner_mobile_clipboard_get"},
                {"cameraAvailable", "__lanner_mobile_camera_available"},
                {"locationAvailable", "__lanner_mobile_location_available"},
                {"bluetoothAvailable", "__lanner_mobile_bluetooth_available"},
                {"appDataPath", "__lanner_mobile_app_data_path"},
                {"documentsPath", "__lanner_mobile_documents_path"},
                {"cachePath", "__lanner_mobile_cache_path"}
            };
            const auto it = mobileCalls.find(field->field);
            if (it == mobileCalls.end()) unsupported("unknown Mobile method '" + field->field + "'", expr->line);
            Instruction call;
            call.op = Opcode::Call;
            call.callee = it->second;
            call.type = exprType(expr);
            if (call.type.kind != ScalarKind::Void) call.result = newTemp();
            for (const auto& arg : expr->args) call.args.push_back(lowerExpr(arg.get()));
            emit(std::move(call));
            return block().instructions.back().result;
        }
        if (field->target->kind == ExprKind::Identifier && field->target->strValue == "Arena" && field->field == "create") {
            if (expr->args.size() != 1) unsupported("Arena.create() takes one argument", expr->line);
            Instruction a;
            a.op = Opcode::ArenaCreate;
            a.type = exprType(expr);
            a.result = newTemp();
            a.lhs = lowerExpr(expr->args[0].get());
            a.rhsType = exprType(expr->args[0].get());
            emit(std::move(a));
            return block().instructions.back().result;
        }
        if (!targetType) unsupported("built-in method target has no type", expr->line);
        if (field->field == "len" && targetType->name == "string" && !targetType->isArray) {
            Instruction call;
            call.op = Opcode::Call;
            call.callee = "__lanner_string_len";
            call.type = exprType(expr);
            call.result = newTemp();
            auto value = lowerExpr(field->target.get());
            if (targetType->isReference) {
                Instruction load;
                load.op = Opcode::LoadIndirect;
                load.type = lowerType(targetType);
                load.result = newTemp();
                load.lhs = value;
                emit(std::move(load));
                value = block().instructions.back().result;
            }
            call.args.push_back(value);
            emit(std::move(call));
            return block().instructions.back().result;
        }
        if (field->field == "len" || field->field == "isEmpty") {
            auto normalized = cloneTypeNode(targetType);
            if (normalized->isReference) normalized->isReference = false;
            if (normalized->isArray && normalized->fixedArraySize) {
                Instruction c;
                c.op = Opcode::ConstInt;
                c.type = field->field == "len" ? Type{ScalarKind::Int,64,false,"usize","i64"}
                                                    : Type{ScalarKind::Bool,1,false,"bool","i1"};
                c.result = newTemp();
                c.intValue = field->field == "len" ? *normalized->fixedArraySize : (*normalized->fixedArraySize == 0 ? 1 : 0);
                c.boolValue = false;
                emit(std::move(c));
                return block().instructions.back().result;
            }
            std::string value = lowerExpr(field->target.get());
            Type targetLowerType = lowerType(normalized.get());
            if (targetType->isReference) {
                Instruction load;
                load.op = Opcode::LoadIndirect;
                load.type = targetLowerType;
                load.result = newTemp();
                load.lhs = value;
                emit(std::move(load));
                value = block().instructions.back().result;
            }
            if (field->field == "len") {
                Instruction ex;
                ex.op = Opcode::ExtractValue;
                ex.type = Type{ScalarKind::Int,64,false,"usize","i64"};
                ex.result = newTemp();
                ex.lhs = value;
                ex.lhsType = targetLowerType;
                ex.aggregateIndex = 1;
                emit(std::move(ex));
                return block().instructions.back().result;
            }
            Instruction len;
            len.op = Opcode::ExtractValue;
            len.type = Type{ScalarKind::Int,64,false,"usize","i64"};
            len.result = newTemp();
            len.lhs = value;
            len.lhsType = targetLowerType;
            len.aggregateIndex = 1;
            emit(std::move(len));
            Instruction zero;
            zero.op = Opcode::ConstInt;
            zero.type = len.type;
            zero.result = newTemp();
            zero.intValue = 0;
            emit(std::move(zero));
            Instruction cmp;
            cmp.op = Opcode::Compare;
            cmp.type = Type{ScalarKind::Bool,1,false,"bool","i1"};
            cmp.result = newTemp();
            cmp.lhs = len.result;
            cmp.rhs = zero.result;
            cmp.lhsType = len.type;
            cmp.rhsType = zero.type;
            cmp.operatorName = "==";
            emit(std::move(cmp));
            return block().instructions.back().result;
        }
        if (field->field == "push") {
            if (expr->args.size() != 1) unsupported("push() takes one argument", expr->line);
            std::string targetValue;
            std::string targetAddress;
            if (field->target->checkedType && field->target->checkedType->isReference) {
                targetAddress = lowerExpr(field->target.get());
                auto targetValueType = cloneTypeNode(field->target->checkedType.get());
                targetValueType->isReference = false;
                Instruction load; load.op=Opcode::LoadIndirect; load.type=lowerType(targetValueType.get()); load.result=newTemp(); load.lhs=targetAddress; emit(std::move(load));
                targetValue = load.result;
            } else {
                targetValue = lowerExpr(field->target.get());
            }
            const auto value = lowerExpr(expr->args[0].get());
            markMovedIfOwned(expr->args[0].get());
            Instruction push;
            push.op = Opcode::DynamicArrayPush;
            auto pushTypeNode = cloneTypeNode(field->target->checkedType.get());
            pushTypeNode->isReference = false;
            push.type = lowerType(pushTypeNode.get());
            push.result = newTemp();
            push.lhs = targetValue;
            push.rhs = value;
            auto elem = arrayElementType(field->target->checkedType.get());
            push.rhsType = lowerType(elem.get());
            push.elementSize = 0;
            emit(std::move(push));
            if (targetAddress.empty() && field->target->kind == ExprKind::Identifier) {
                const auto slot = lookupLocal(field->target->strValue);
                if (!slot.empty()) {
                    Instruction store;
                    store.op = Opcode::StoreLocal;
                    store.type = push.type;
                    store.slot = slot;
                    store.lhs = block().instructions.back().result;
                    emit(std::move(store));
                    initializedSlots.insert(slot);
                    destroyedSlots.erase(slot);
                    return {};
                }
            } else if (targetAddress.empty() && field->target->kind != ExprKind::Identifier) {
                unsupported("HIR push target must be an owned array or &mut []T", expr->line);
            } else {
                Instruction store; store.op=Opcode::StoreIndirect; store.type=push.type; store.lhs=targetAddress; store.rhs=block().instructions.back().result; emit(std::move(store));
            }
            return {};
        }
        unsupported("unsupported built-in method '" + field->field + "'", expr->line);
    }
    if (expr->callee->kind != ExprKind::Identifier) unsupported("HIR calls require a direct function name", expr->line);
    const std::string name = expr->callee->strValue;
    if (name == "print") {
        if (expr->args.size() != 1) unsupported("print() takes one argument", expr->line);
        const auto* argExpr = expr->args[0].get();
        if (!argExpr->checkedType) unsupported("print() argument has no checked type", expr->line);
        const Type sourceType = lowerType(argExpr->checkedType.get());

        Instruction call;
        call.op = Opcode::Call;
        call.type = {ScalarKind::Void, 0, false, "void", "void"};
        if (sourceType.kind == ScalarKind::String) {
            call.callee = "__lanner_print_string";
            call.args.push_back(lowerExpr(argExpr));
        } else if (sourceType.kind == ScalarKind::Bool) {
            call.callee = "__lanner_print_bool";
            call.args.push_back(lowerExpr(argExpr));
        } else if (sourceType.kind == ScalarKind::Float) {
            call.callee = "__lanner_print_f64";
            auto value = lowerExpr(argExpr);
            if (sourceType.bits != 64) {
                Instruction cast;
                cast.op = Opcode::Cast;
                cast.type = {ScalarKind::Float, 64, false, "f64", "double"};
                cast.sourceType = sourceType;
                cast.lhs = value;
                cast.result = newTemp();
                emit(std::move(cast));
                value = block().instructions.back().result;
            }
            call.args.push_back(value);
        } else if (sourceType.kind == ScalarKind::Int) {
            call.callee = sourceType.isSigned ? "__lanner_print_i64" : "__lanner_print_u64";
            auto value = lowerExpr(argExpr);
            if (sourceType.bits != 64) {
                Instruction cast;
                cast.op = Opcode::Cast;
                cast.type = {ScalarKind::Int, 64, sourceType.isSigned, sourceType.isSigned ? "i64" : "u64", "i64"};
                cast.sourceType = sourceType;
                cast.lhs = value;
                cast.result = newTemp();
                emit(std::move(cast));
                value = block().instructions.back().result;
            }
            call.args.push_back(value);
        } else {
            unsupported("print() argument type is not printable", expr->line);
        }
        emit(std::move(call));
        return {};
    }
    if (name == "readFile" || name == "writeStdout" || name == "writeRaw" || name == "writeIntRaw" || name == "writeByteRaw" || name == "printInt" || name == "stringLen" || name == "getEnv") {
        Instruction call;
        call.op = Opcode::Call;
        call.callee = "__lanner_" + name;
        if (name == "readFile") call.callee = "__lanner_read_file";
        if (name == "writeStdout") call.callee = "__lanner_write_stdout";
        if (name == "writeRaw") call.callee = "__lanner_write_raw";
        if (name == "writeIntRaw") call.callee = "__lanner_write_i64_raw";
        if (name == "writeByteRaw") call.callee = "__lanner_write_byte_raw";
        if (name == "printInt") call.callee = "__lanner_print_i64";
        if (name == "stringLen") call.callee = "__lanner_string_len";
        if (name == "getEnv") call.callee = "__lanner_getenv";
        call.type = exprType(expr);
        if (call.type.kind != ScalarKind::Void) call.result = newTemp();
        for (const auto& arg : expr->args) call.args.push_back(lowerExpr(arg.get()));
        emit(std::move(call));
        return block().instructions.back().result;
    }
    auto fnIt = functions.find(name);
    if (fnIt == functions.end()) unsupported("unknown function '" + name + "'", expr->line);
    const auto* fn = fnIt->second;
    if (fn->params.size() != expr->args.size()) unsupported("wrong argument count in call to '" + name + "'", expr->line);
    Instruction call;
    call.op = Opcode::Call;
    call.callee = name;
    call.type = exprType(expr);
    if (call.type.kind != ScalarKind::Void) call.result = newTemp();
    for (std::size_t i = 0; i < expr->args.size(); ++i) {
        call.args.push_back(lowerExpr(expr->args[i].get()));
        if (!fn->params[i].type->isReference && fn->params[i].type->name != "View" && fn->params[i].type->name != "EditView") {
            markMovedIfOwned(expr->args[i].get());
        }
    }
    emit(std::move(call));
    return block().instructions.back().result;
}

std::string Lowerer::lowerImplicitOptionalWrap(const Expr* expr) {
    if (!expr || !expr->checkedType || !expr->checkedType->isOptional) {
        unsupported("invalid implicit optional conversion", expr ? expr->line : 0);
    }
    auto* mutableExpr = const_cast<Expr*>(expr);
    mutableExpr->implicitOptionalWrap = false;
    const auto resultType = exprType(expr);
    auto savedType = std::move(mutableExpr->checkedType);
    auto payloadTypeNode = cloneTypeNode(savedType.get());
    payloadTypeNode->isOptional = false;
    mutableExpr->checkedType = cloneTypeNode(payloadTypeNode.get());
    const auto payload = lowerExpr(expr);
    mutableExpr->checkedType = std::move(savedType);
    mutableExpr->implicitOptionalWrap = true;

    Instruction zero;
    zero.op = Opcode::ZeroValue;
    zero.type = resultType;
    zero.result = newTemp();
    const std::string zeroValue = zero.result;
    emit(std::move(zero));

    Instruction tag;
    tag.op = Opcode::ConstBool;
    tag.type = Type{ScalarKind::Bool, 1, false, "bool", "i1"};
    tag.result = newTemp();
    tag.boolValue = true;
    const std::string tagValue = tag.result;
    emit(std::move(tag));

    Instruction tagged;
    tagged.op = Opcode::InsertValue;
    tagged.type = resultType;
    tagged.result = newTemp();
    tagged.lhs = zeroValue;
    tagged.rhs = tagValue;
    tagged.lhsType = resultType;
    tagged.rhsType = tag.type;
    tagged.aggregateIndex = 0;
    tagged.aggregateIndexConstant = true;
    const std::string taggedValue = tagged.result;
    emit(std::move(tagged));

    Instruction payloadInsert;
    payloadInsert.op = Opcode::InsertValue;
    payloadInsert.type = resultType;
    payloadInsert.result = newTemp();
    payloadInsert.lhs = taggedValue;
    payloadInsert.rhs = payload;
    payloadInsert.lhsType = resultType;
    auto payloadType = cloneTypeNode(expr->checkedType.get());
    payloadType->isOptional = false;
    payloadInsert.rhsType = lowerType(payloadType.get());
    payloadInsert.aggregateIndex = 1;
    payloadInsert.aggregateIndexConstant = true;
    emit(std::move(payloadInsert));
    return block().instructions.back().result;
}

std::string Lowerer::lowerExpr(const Expr* expr) {
    if (!expr) unsupported("null expression", 0);
    if (expr->implicitOptionalWrap) return lowerImplicitOptionalWrap(expr);
    switch (expr->kind) {
        case ExprKind::StringLit: {
            Instruction c;
            c.op = Opcode::ConstString;
            c.type = exprType(expr);
            c.result = newTemp();
            c.stringValue = expr->strValue;
            emit(std::move(c));
            return block().instructions.back().result;
        }
        case ExprKind::IntLit: {
            Instruction c;
            c.op = Opcode::ConstInt; c.type = exprType(expr); c.result = newTemp();
            c.intValue = parseUInt(expr->strValue);
            emit(std::move(c)); return block().instructions.back().result;
        }
        case ExprKind::FloatLit: {
            Instruction c;
            c.op = Opcode::ConstFloat; c.type = exprType(expr); c.result = newTemp();
            c.floatValue = std::stod(expr->strValue);
            emit(std::move(c)); return block().instructions.back().result;
        }
        case ExprKind::BoolLit: {
            Instruction c;
            c.op = Opcode::ConstBool; c.type = exprType(expr); c.result = newTemp();
            c.boolValue = expr->strValue == "true";
            emit(std::move(c)); return block().instructions.back().result;
        }
        case ExprKind::NoneLit: {
            if (!expr->checkedType || !expr->checkedType->isOptional) {
                unsupported("none requires an optional type", expr->line);
            }
            Instruction zero;
            zero.op = Opcode::ZeroValue;
            zero.type = exprType(expr);
            zero.result = newTemp();
            emit(std::move(zero));
            return block().instructions.back().result;
        }
        case ExprKind::OkLit:
        case ExprKind::ErrLit: {
            if (!expr->checkedType || expr->checkedType->name != "Result" || expr->checkedType->generics.size() != 2) {
                unsupported("Ok/Err requires a Result type", expr->line);
            }
            const bool ok = expr->kind == ExprKind::OkLit;
            const auto resultType = exprType(expr);
            Instruction zero;
            zero.op = Opcode::ZeroValue;
            zero.type = resultType;
            zero.result = newTemp();
            const std::string zeroValue = zero.result;
            emit(std::move(zero));

            Instruction tag;
            tag.op = Opcode::ConstBool;
            tag.type = Type{ScalarKind::Bool, 1, false, "bool", "i1"};
            tag.result = newTemp();
            tag.boolValue = ok;
            const std::string tagValue = tag.result;
            emit(std::move(tag));

            Instruction tagInsert;
            tagInsert.op = Opcode::InsertValue;
            tagInsert.type = resultType;
            tagInsert.result = newTemp();
            tagInsert.lhs = zeroValue;
            tagInsert.rhs = tagValue;
            tagInsert.lhsType = resultType;
            tagInsert.rhsType = tag.type;
            tagInsert.aggregateIndex = 0;
            tagInsert.aggregateIndexConstant = true;
            const std::string tagInsertValue = tagInsert.result;
            emit(std::move(tagInsert));

            Instruction payloadInsert;
            payloadInsert.op = Opcode::InsertValue;
            payloadInsert.type = resultType;
            payloadInsert.result = newTemp();
            payloadInsert.lhs = tagInsertValue;
            payloadInsert.rhs = lowerExpr(expr->value.get());
            markMovedIfOwned(expr->value.get());
            payloadInsert.lhsType = resultType;
            payloadInsert.rhsType = exprType(expr->value.get());
            payloadInsert.aggregateIndex = ok ? 1 : 2;
            payloadInsert.aggregateIndexConstant = true;
            emit(std::move(payloadInsert));
            return block().instructions.back().result;
        }
        case ExprKind::IsMatch: {
            if (!expr->left || expr->left->checkedType == nullptr || expr->left->checkedType->name != "Result") {
                unsupported("Result match requires a Result value", expr->line);
            }
            const auto value = lowerExpr(expr->left.get());
            Instruction tag;
            tag.op = Opcode::ExtractValue;
            tag.type = Type{ScalarKind::Bool, 1, false, "bool", "i1"};
            tag.result = newTemp();
            tag.lhs = value;
            tag.lhsType = exprType(expr->left.get());
            tag.aggregateIndex = 0;
            const std::string tagValue = tag.result;
            emit(std::move(tag));
            if (expr->matchKind == "Ok") return tagValue;
            Instruction neg;
            neg.op = Opcode::Unary;
            neg.type = Type{ScalarKind::Bool, 1, false, "bool", "i1"};
            neg.result = newTemp();
            neg.lhs = tagValue;
            neg.operatorName = "!";
            emit(std::move(neg));
            return block().instructions.back().result;
        }
        case ExprKind::Identifier: {
            const auto slot = lookupLocal(expr->strValue);
            if (!slot.empty()) {
                const auto localIt = current.locals.find(slot);
                if (localIt == current.locals.end()) unsupported("unknown HIR local slot '" + slot + "'", expr->line);
                Instruction load;
                load.op = Opcode::LoadLocal;
                load.type = localIt->second;
                load.result = newTemp();
                load.slot = slot;
                emit(std::move(load));
                const bool transparentReferenceRead =
                    (localIt->second.memory == MemoryKind::ExclusiveReference ||
                     localIt->second.memory == MemoryKind::SharedReference) &&
                    expr->checkedType && !expr->checkedType->isReference;
                if (transparentReferenceRead) {
                    Instruction deref;
                    deref.op = Opcode::LoadIndirect;
                    deref.type = exprType(expr);
                    deref.result = newTemp();
                    deref.lhs = block().instructions.back().result;
                    emit(std::move(deref));
                    return block().instructions.back().result;
                }
                return block().instructions.back().result;
            }
            const auto* comptimeValue = lookupComptime(expr->strValue);
            if (comptimeValue) {
                Instruction constant;
                constant.type = exprType(expr);
                constant.result = newTemp();
                if (std::holds_alternative<std::int64_t>(*comptimeValue)) {
                    constant.op = Opcode::ConstInt;
                    constant.intValue = static_cast<std::uint64_t>(std::get<std::int64_t>(*comptimeValue));
                } else if (std::holds_alternative<double>(*comptimeValue)) {
                    constant.op = Opcode::ConstFloat;
                    constant.floatValue = std::get<double>(*comptimeValue);
                } else if (std::holds_alternative<bool>(*comptimeValue)) {
                    constant.op = Opcode::ConstBool;
                    constant.boolValue = std::get<bool>(*comptimeValue);
                } else {
                    constant.op = Opcode::ConstString;
                    constant.stringValue = std::get<std::string>(*comptimeValue);
                }
                emit(std::move(constant));
                return block().instructions.back().result;
            }
            unsupported("unknown local or comptime value '" + expr->strValue + "'", expr->line);
        }
        case ExprKind::FieldAccess: {
            if (expr->target->kind == ExprKind::Identifier && enums.count(expr->target->strValue)) {
                return lowerFieldEnum(expr);
            }
            const auto* targetType = expr->target->checkedType.get();
            if (targetType && targetType->isReference) {
                const auto address = lowerLValueAddress(expr);
                Instruction load;
                load.op = Opcode::LoadIndirect;
                load.type = exprType(expr);
                load.result = newTemp();
                load.lhs = address;
                emit(std::move(load));
                return block().instructions.back().result;
            }
            return lowerStructFieldValue(expr);
        }
        case ExprKind::UnaryOp: {
            if (expr->op == "&" || expr->op == "&mut") {
                return lowerLValueAddress(expr->value.get());
            }
            const std::string value = lowerExpr(expr->value.get());
            Instruction u;
            u.op = Opcode::Unary; u.type = exprType(expr); u.result = newTemp(); u.lhs = value; u.operatorName = expr->op;
            emit(std::move(u)); return block().instructions.back().result;
        }
        case ExprKind::BinaryOp: {
            if (expr->op == "&&" || expr->op == "||") {
                const std::string lhs = lowerExpr(expr->left.get());
                const std::string slot = "__lanner_sc_" + std::to_string(++tempCounter);
                current.locals.emplace(slot, Type{ScalarKind::Bool, 1, false, "bool", "i1"});

                const std::string rhsLabel = newBlockLabel(expr->op == "&&" ? "sc_rhs" : "sc_false");
                const std::string shortLabel = newBlockLabel(expr->op == "&&" ? "sc_false" : "sc_true");
                const std::string endLabel = newBlockLabel("sc_end");

                Instruction branch;
                branch.op = Opcode::CondBranch;
                branch.lhs = lhs;
                branch.trueLabel = expr->op == "&&" ? rhsLabel : shortLabel;
                branch.falseLabel = expr->op == "&&" ? shortLabel : rhsLabel;
                emit(std::move(branch));

                current.blocks.push_back(BasicBlock{rhsLabel, {}});
                currentBlock = current.blocks.size() - 1;
                const std::string rhs = lowerExpr(expr->right.get());
                Instruction rhsStore;
                rhsStore.op = Opcode::StoreLocal; rhsStore.type = Type{ScalarKind::Bool, 1, false, "bool", "i1"};
                rhsStore.slot = slot; rhsStore.lhs = rhs;
                emit(std::move(rhsStore));
                ensureBranchTo(endLabel);

                current.blocks.push_back(BasicBlock{shortLabel, {}});
                currentBlock = current.blocks.size() - 1;
                Instruction shortStore;
                shortStore.op = Opcode::StoreLocal; shortStore.type = Type{ScalarKind::Bool, 1, false, "bool", "i1"};
                shortStore.slot = slot;
                const std::string shortValue = newTemp();
                Instruction c; c.op = Opcode::ConstBool; c.type = Type{ScalarKind::Bool, 1, false, "bool", "i1"};
                c.result = shortValue; c.boolValue = expr->op == "||"; emit(std::move(c));
                shortStore.lhs = shortValue;
                emit(std::move(shortStore));
                ensureBranchTo(endLabel);

                current.blocks.push_back(BasicBlock{endLabel, {}});
                currentBlock = current.blocks.size() - 1;
                Instruction load; load.op = Opcode::LoadLocal; load.type = Type{ScalarKind::Bool, 1, false, "bool", "i1"};
                load.result = newTemp(); load.slot = slot; emit(std::move(load));
                return block().instructions.back().result;
            }
            const std::string lhs = lowerExpr(expr->left.get());
            const std::string rhs = lowerExpr(expr->right.get());
            Instruction b;
            b.op = isCompareOp(expr->op) ? Opcode::Compare : Opcode::Binary;
            b.type = exprType(expr); b.result = newTemp(); b.lhs = lhs; b.rhs = rhs;
            b.lhsType = exprType(expr->left.get());
            b.rhsType = exprType(expr->right.get());
            b.operatorName = expr->op;
            emit(std::move(b)); return block().instructions.back().result;
        }
        case ExprKind::Cast: {
            const std::string value = lowerExpr(expr->value.get());
            Instruction c;
            c.op = Opcode::Cast; c.type = exprType(expr); c.result = newTemp(); c.lhs = value;
            c.sourceType = exprType(expr->value.get());
            emit(std::move(c)); return block().instructions.back().result;
        }
        case ExprKind::Call:
            return lowerCall(expr);
        case ExprKind::ArenaAlloc: {
            if (!expr->arena || expr->arena->kind != ExprKind::Identifier) unsupported("arena target must be an identifier", expr->line);
            const auto arena = addressOfLocal(expr->arena->strValue, expr->line);
            const auto arrayType = exprType(expr);
            Instruction zero; zero.op=Opcode::ZeroValue; zero.type=arrayType; zero.result=newTemp(); emit(std::move(zero));
            std::string value = block().instructions.back().result;
            Instruction tagged; tagged.op=Opcode::InsertValue; tagged.type=arrayType; tagged.result=newTemp(); tagged.lhs=value; tagged.rhs=arena;
            tagged.lhsType=arrayType; tagged.rhsType=Type{ScalarKind::Aggregate,0,false,"ptr","ptr"}; tagged.aggregateIndex=3; tagged.aggregateIndexConstant=true; emit(std::move(tagged));
            value = block().instructions.back().result;
            for (const auto& arg : expr->value->args) {
                Instruction push; push.op=Opcode::DynamicArrayPush; push.type=arrayType; push.result=newTemp(); push.lhs=value; push.rhs=lowerExpr(arg.get());
                auto elem=arrayElementType(expr->value->checkedType.get()); push.rhsType=lowerType(elem.get()); emit(std::move(push)); value=block().instructions.back().result;
            }
            return value;
        }
        case ExprKind::Slice: {
            if (expr->args.size() != 2) unsupported("slice requires lower and upper bounds", expr->line);
            const auto* sourceType = expr->target->checkedType.get();
            const auto* viewType = expr->checkedType.get();
            if (!viewType || (viewType->name != "View" && viewType->name != "EditView")) unsupported("slice does not produce a View", expr->line);
            std::string base;
            std::string limit;
            auto normalized = cloneTypeNode(sourceType);
            if (normalized->isReference) normalized->isReference = false;
            if (normalized->isArray && normalized->fixedArraySize) {
                base = sourceType && sourceType->isReference ? lowerExpr(expr->target.get()) : lowerLValueAddress(expr->target.get());
                limit = std::to_string(*normalized->fixedArraySize);
            } else if (normalized->isArray) {
                const auto addr = sourceType && sourceType->isReference ? lowerExpr(expr->target.get()) : lowerLValueAddress(expr->target.get());
                const auto arr = newTemp();
                Instruction load; load.op=Opcode::LoadIndirect; load.type=lowerType(normalized.get()); load.result=arr; load.lhs=addr; emit(std::move(load));
                Instruction data; data.op=Opcode::ExtractValue; data.type=Type{ScalarKind::Aggregate,0,false,"ptr","ptr"}; data.result=newTemp(); data.lhs=arr; data.lhsType=load.type; data.aggregateIndex=0; emit(std::move(data));
                base=data.result;
                Instruction len; len.op=Opcode::ExtractValue; len.type=Type{ScalarKind::Int,64,false,"usize","i64"}; len.result=newTemp(); len.lhs=arr; len.lhsType=load.type; len.aggregateIndex=1; emit(std::move(len));
                limit=len.result;
            } else if (normalized->name == "View" || normalized->name == "EditView") {
                const auto vv = lowerExpr(expr->target.get());
                Instruction data; data.op=Opcode::ExtractValue; data.type=Type{ScalarKind::Aggregate,0,false,"ptr","ptr"}; data.result=newTemp(); data.lhs=vv; data.lhsType=exprType(expr->target.get()); data.aggregateIndex=0; emit(std::move(data));
                base=data.result;
                Instruction len; len.op=Opcode::ExtractValue; len.type=Type{ScalarKind::Int,64,false,"usize","i64"}; len.result=newTemp(); len.lhs=vv; len.lhsType=exprType(expr->target.get()); len.aggregateIndex=1; emit(std::move(len));
                limit=len.result;
            } else unsupported("unsupported slice source", expr->line);
            const auto low = expr->args[0] ? lowerExpr(expr->args[0].get()) : std::string("0");
            const auto high = expr->args[1] ? lowerExpr(expr->args[1].get()) : limit;
            // Slice validity uses the same unsigned representation as usize.
            Instruction ordered; ordered.op=Opcode::Compare; ordered.type=Type{ScalarKind::Bool,1,false,"bool","i1"}; ordered.result=newTemp(); ordered.lhs=low; ordered.rhs=high; ordered.lhsType=expr->args[0] ? exprType(expr->args[0].get()) : Type{ScalarKind::Int,64,false,"usize","i64"}; ordered.rhsType=expr->args[1] ? exprType(expr->args[1].get()) : Type{ScalarKind::Int,64,false,"usize","i64"}; ordered.operatorName="<="; emit(std::move(ordered));
            Instruction inside; inside.op=Opcode::Compare; inside.type=ordered.type; inside.result=newTemp(); inside.lhs=high; inside.rhs=limit; inside.lhsType=ordered.rhsType; inside.rhsType=Type{ScalarKind::Int,64,false,"usize","i64"}; inside.operatorName="<="; emit(std::move(inside));
            Instruction valid; valid.op=Opcode::Binary; valid.type=ordered.type; valid.result=newTemp(); valid.lhs=ordered.result; valid.rhs=inside.result; valid.lhsType=ordered.type; valid.rhsType=inside.type; valid.operatorName="&"; emit(std::move(valid));
            const auto good = newBlockLabel("slice_ok"); const auto bad = newBlockLabel("slice_bad"); const auto join = newBlockLabel("slice_join");
            Instruction br; br.op=Opcode::CondBranch; br.lhs=valid.result; br.trueLabel=good; br.falseLabel=bad; emit(std::move(br));
            current.blocks.push_back(BasicBlock{bad,{}}); currentBlock=current.blocks.size()-1; Instruction trap; trap.op=Opcode::Call; trap.callee="llvm.trap"; trap.type=Type{ScalarKind::Void,0,false,"void",""}; emit(std::move(trap)); Instruction un; un.op=Opcode::Branch; un.targetLabel=join; emit(std::move(un));
            current.blocks.push_back(BasicBlock{good,{}}); currentBlock=current.blocks.size()-1; Instruction p; p.op=Opcode::AggregateIndex; p.type=lowerType(viewType->generics[0].get()); p.result=newTemp(); p.lhs=base; p.rhs=low; p.rhsType=Type{ScalarKind::Int,64,false,"usize","i64"}; p.sourceType = (normalized->isArray && normalized->fixedArraySize) ? lowerType(normalized.get()) : Type{ScalarKind::Aggregate,0,false,"slice_base","[1 x "+p.type.llvmName+"]"};
            p.aggregateIndexConstant=false; emit(std::move(p)); Instruction length; length.op=Opcode::Binary; length.type=Type{ScalarKind::Int,64,false,"usize","i64"}; length.result=newTemp(); length.lhs=high; length.rhs=low; length.lhsType=length.type; length.rhsType=length.type; length.operatorName="-"; emit(std::move(length)); Instruction v0; v0.op=Opcode::ZeroValue; v0.type=exprType(expr); v0.result=newTemp(); emit(std::move(v0)); Instruction v1; v1.op=Opcode::InsertValue; v1.type=exprType(expr); v1.result=newTemp(); v1.lhs=v0.result; v1.rhs=p.result; v1.lhsType=v0.type; v1.rhsType=Type{ScalarKind::Aggregate,0,false,"ptr","ptr"}; v1.aggregateIndex=0; v1.aggregateIndexConstant=true; emit(std::move(v1)); Instruction v2; v2.op=Opcode::InsertValue; v2.type=exprType(expr); v2.result=newTemp(); v2.lhs=v1.result; v2.rhs=length.result; v2.lhsType=v1.type; v2.rhsType=length.type; v2.aggregateIndex=1; v2.aggregateIndexConstant=true; emit(std::move(v2)); ensureBranchTo(join);
            current.blocks.push_back(BasicBlock{join,{}}); currentBlock=current.blocks.size()-1;
            return v2.result;
        }
        case ExprKind::ArrayLit: {
            if (!expr->checkedType || !expr->checkedType->isArray) unsupported("array literal has no array type", expr->line);
            const auto arrayType = exprType(expr);
            if (!expr->checkedType->fixedArraySize) {
                Instruction zero; zero.op=Opcode::ZeroValue; zero.type=arrayType; zero.result=newTemp(); emit(std::move(zero));
                std::string value=block().instructions.back().result;
                for (const auto& arg : expr->args) {
                    Instruction push; push.op=Opcode::DynamicArrayPush; push.type=arrayType; push.result=newTemp(); push.lhs=value; push.rhs=lowerExpr(arg.get());
                    auto elem=arrayElementType(expr->checkedType.get()); push.rhsType=lowerType(elem.get()); emit(std::move(push)); value=block().instructions.back().result;
                }
                return value;
            }
            if (expr->args.empty()) {
                Instruction zero;
                zero.op = Opcode::ZeroValue;
                zero.type = arrayType;
                zero.result = newTemp();
                emit(std::move(zero));
                return block().instructions.back().result;
            }
            std::string value;
            for (std::size_t i = 0; i < expr->args.size(); ++i) {
                Instruction ins;
                ins.op = Opcode::InsertValue;
                ins.type = arrayType;
                ins.result = newTemp();
                ins.lhs = value;
                ins.rhs = lowerExpr(expr->args[i].get());
                ins.lhsType = arrayType;
                ins.rhsType = exprType(expr->args[i].get());
                ins.aggregateIndex = i;
                ins.aggregateIndexConstant = true;
                emit(std::move(ins));
                value = block().instructions.back().result;
                markMovedIfOwned(expr->args[i].get());
            }
            return value;
        }
        case ExprKind::StructLit: {
            auto it = structs.find(expr->structName);
            if (it == structs.end()) unsupported("unknown struct '" + expr->structName + "'", expr->line);
            const auto structType = exprType(expr);
            std::map<std::string, const Expr*> supplied;
            for (const auto& field : expr->fields) supplied[field.first] = field.second.get();
            std::string value;
            for (std::size_t i = 0; i < it->second->fields.size(); ++i) {
                const auto fieldIt = supplied.find(it->second->fields[i].name);
                if (fieldIt == supplied.end()) unsupported("missing struct literal field", expr->line);
                Instruction ins;
                ins.op = Opcode::InsertValue;
                ins.type = structType;
                ins.result = newTemp();
                ins.lhs = value;
                ins.rhs = lowerExpr(fieldIt->second);
                ins.lhsType = structType;
                ins.rhsType = exprType(fieldIt->second);
                ins.aggregateIndex = i;
                ins.aggregateIndexConstant = true;
                emit(std::move(ins));
                value = block().instructions.back().result;
                markMovedIfOwned(fieldIt->second);
            }
            return value;
        }
        case ExprKind::Index: {
            const auto address = lowerAggregateIndexAddress(expr);
            Instruction load;
            load.op = Opcode::LoadIndirect;
            load.type = exprType(expr);
            load.result = newTemp();
            load.lhs = address;
            emit(std::move(load));
            return block().instructions.back().result;
        }
        default:
            unsupported("expression kind is not yet supported by HIR", expr->line);
    }
}

void Lowerer::lowerStmt(const Stmt* stmt) {
    if (!stmt) return;
    switch (stmt->kind) {
        case StmtKind::ExprStmt:
            (void)lowerExpr(stmt->expr.get());
            return;
        case StmtKind::Assign: {
            const Type type = stmt->declaredType ? declaredType(stmt->declaredType.get()) : exprType(stmt->assignValue.get());
            if (!stmt->expr) {
                std::string slot = stmt->declaredType ? declareLocal(stmt->assignTarget) : lookupLocal(stmt->assignTarget);
                if (slot.empty()) slot = declareLocal(stmt->assignTarget);
                const bool existingReference = !stmt->declaredType && !slot.empty() &&
                    current.locals.count(slot) &&
                    (current.locals.at(slot).memory == MemoryKind::ExclusiveReference ||
                     current.locals.at(slot).memory == MemoryKind::SharedReference);
                const std::string value = lowerExpr(stmt->assignValue.get());
                if (existingReference) {
                    // Bare identifier assignment through &mut T is an indirect store.
                    // Build the pointee address directly from the reference slot because
                    // the AST uses assignTarget rather than an Expr target here.
                    const std::string bindingAddr = addressOfLocal(stmt->assignTarget, stmt->line);
                    Instruction refAddr;
                    refAddr.op = Opcode::LoadIndirect;
                    refAddr.type = Type{ScalarKind::Aggregate, 0, false, "&ref-pointee", "ptr"};
                    refAddr.result = newTemp();
                    refAddr.lhs = bindingAddr;
                    emit(std::move(refAddr));
                    const std::string pointeeAddr = block().instructions.back().result;
                    emitDestroyAt(pointeeAddr, type);
                    Instruction store;
                    store.op = Opcode::StoreIndirect;
                    store.type = type;
                    store.lhs = pointeeAddr;
                    store.rhs = value;
                    emit(std::move(store));
                    markMovedIfOwned(stmt->assignValue.get());
                } else {
                    current.locals.emplace(slot, type);
                    emitStoreLocal(slot, type, value, stmt->assignValue.get());
                }
                return;
            }

            if (stmt->expr->kind == ExprKind::FieldAccess) {
                const auto address = lowerLValueAddress(stmt->expr.get());
                const auto value = lowerExpr(stmt->assignValue.get());
                const auto valueType = exprType(stmt->expr.get());
                emitDestroyAt(address, valueType);
                Instruction store;
                store.op = Opcode::StoreIndirect;
                store.type = valueType;
                store.lhs = address;
                store.rhs = value;
                emit(std::move(store));
                markMovedIfOwned(stmt->assignValue.get());
                return;
            }

            if (stmt->expr->kind == ExprKind::Index) {
                const auto address = lowerAggregateIndexAddress(stmt->expr.get());
                const auto value = lowerExpr(stmt->assignValue.get());
                const auto valueType = exprType(stmt->expr.get());
                emitDestroyAt(address, valueType);
                Instruction store;
                store.op = Opcode::StoreIndirect;
                store.type = valueType;
                store.lhs = address;
                store.rhs = value;
                emit(std::move(store));
                markMovedIfOwned(stmt->assignValue.get());
                return;
            }
            unsupported("complex lvalue assignment is not yet supported by HIR", stmt->line);
        }
        case StmtKind::ConstAssign: {
            if (stmt->expr) unsupported("complex const assignment is not yet supported by HIR", stmt->line);
            const std::string slot = declareLocal(stmt->assignTarget);
            const Type type = stmt->declaredType ? declaredType(stmt->declaredType.get()) : exprType(stmt->assignValue.get());
            current.locals.emplace(slot, type);
            const std::string value = lowerExpr(stmt->assignValue.get());
            emitStoreLocal(slot, type, value, stmt->assignValue.get());
            return;
        }
        case StmtKind::Return: {
            if (!stmt->expr) {
                cleanupScopes();
                Instruction r; r.op = Opcode::Return; emit(std::move(r));
            } else {
                const auto value = lowerExpr(stmt->expr.get());
                markMovedIfOwned(stmt->expr.get());
                cleanupScopes();
                Instruction r; r.op = Opcode::Return; r.lhs = value; r.type = current.returnType; emit(std::move(r));
            }
            return;
        }
        case StmtKind::If: {
            const std::string cond = lowerExpr(stmt->expr.get());
            const std::string thenLabel = newBlockLabel("then");
            const bool hasElse = !stmt->elseBody.empty();
            const std::string elseLabel = hasElse ? newBlockLabel("else") : "";
            const std::string endLabel = newBlockLabel("if_end");

            Instruction br;
            br.op = Opcode::CondBranch;
            br.lhs = cond;
            br.trueLabel = thenLabel;
            br.falseLabel = hasElse ? elseLabel : endLabel;
            emit(std::move(br));

            current.blocks.push_back(BasicBlock{thenLabel, {}});
            currentBlock = current.blocks.size() - 1;
            pushScope();
            lowerBlock(stmt->body);
            if (!terminated()) cleanupCurrentScope();
            popScope();
            const bool thenTerminated = terminated();
            if (!thenTerminated) ensureBranchTo(endLabel);

            bool elseTerminated = true;
            if (hasElse) {
                current.blocks.push_back(BasicBlock{elseLabel, {}});
                currentBlock = current.blocks.size() - 1;
                pushScope();
                lowerBlock(stmt->elseBody);
                if (!terminated()) cleanupCurrentScope();
                popScope();
                elseTerminated = terminated();
                if (!elseTerminated) ensureBranchTo(endLabel);
            }

            if (!hasElse || !thenTerminated || !elseTerminated) {
                current.blocks.push_back(BasicBlock{endLabel, {}});
                currentBlock = current.blocks.size() - 1;
            }
            return;
        }
        case StmtKind::While: {
            const std::string condLabel = newBlockLabel("while_cond");
            const std::string bodyLabel = newBlockLabel("while_body");
            const std::string exitLabel = newBlockLabel("while_exit");
            ensureBranchTo(condLabel);

            current.blocks.push_back(BasicBlock{condLabel, {}});
            currentBlock = current.blocks.size() - 1;
            const std::string cond = lowerExpr(stmt->expr.get());
            Instruction br;
            br.op = Opcode::CondBranch; br.lhs = cond; br.trueLabel = bodyLabel; br.falseLabel = exitLabel;
            emit(std::move(br));

            current.blocks.push_back(BasicBlock{bodyLabel, {}});
            currentBlock = current.blocks.size() - 1;
            pushScope();
            loopScopeDepths.push_back(localScopes.size() - 1);
            breakTargets.push_back(exitLabel);
            continueTargets.push_back(condLabel);
            lowerBlock(stmt->body);
            if (!terminated()) cleanupCurrentScope();
            continueTargets.pop_back();
            breakTargets.pop_back();
            loopScopeDepths.pop_back();
            popScope();
            ensureBranchTo(condLabel);

            current.blocks.push_back(BasicBlock{exitLabel, {}});
            currentBlock = current.blocks.size() - 1;
            return;
        }
        case StmtKind::Break: {
            if (breakTargets.empty()) unsupported("break outside loop", stmt->line);
            cleanupScopesFrom(loopScopeDepths.back());
            Instruction br; br.op = Opcode::Branch; br.targetLabel = breakTargets.back(); emit(std::move(br)); return;
        }
        case StmtKind::Continue: {
            if (continueTargets.empty()) unsupported("continue outside loop", stmt->line);
            cleanupScopesFrom(loopScopeDepths.back());
            Instruction br; br.op = Opcode::Branch; br.targetLabel = continueTargets.back(); emit(std::move(br)); return;
        }
        case StmtKind::Guard: {
            if (!stmt->guardCondition || stmt->guardCondition->kind != ExprKind::IsMatch) {
                unsupported("HIR guards currently require a Result is Ok/Err condition", stmt->line);
            }
            const Expr* match = stmt->guardCondition.get();
            if (!match->left || !match->left->checkedType || match->left->checkedType->name != "Result") {
                unsupported("HIR guard requires a Result value", stmt->line);
            }
            const auto resultValue = lowerExpr(match->left.get());
            std::string resultAddress;
            if (match->left->kind == ExprKind::Identifier) {
                resultAddress = lowerLValueAddress(match->left.get());
            } else {
                const std::string tempSlot = "__lanner_match_result." + std::to_string(++tempCounter);
                const Type resultType = exprType(match->left.get());
                current.locals[tempSlot] = resultType;
                localScopes.back()[tempSlot] = tempSlot;
                scopeOrder.back().push_back(tempSlot);
                Instruction save;
                save.op = Opcode::StoreLocal;
                save.type = resultType;
                save.slot = tempSlot;
                save.lhs = resultValue;
                emit(std::move(save));
                initializedSlots.insert(tempSlot);
                resultAddress = addressOfLocal(tempSlot, stmt->line);
            }
            Instruction tag;
            tag.op = Opcode::ExtractValue;
            tag.type = Type{ScalarKind::Bool, 1, false, "bool", "i1"};
            tag.result = newTemp();
            tag.lhs = resultValue;
            tag.lhsType = exprType(match->left.get());
            tag.aggregateIndex = 0;
            const std::string tagValue = tag.result;
            emit(std::move(tag));

            const std::string thenLabel = newBlockLabel("guard_then");
            const std::string endLabel = newBlockLabel("guard_end");
            Instruction br;
            br.op = Opcode::CondBranch;
            br.lhs = match->matchKind == "Ok" ? tagValue : (newTemp());
            if (match->matchKind == "Err") {
                Instruction neg;
                neg.op = Opcode::Unary;
                neg.type = Type{ScalarKind::Bool, 1, false, "bool", "i1"};
                neg.result = br.lhs;
                neg.lhs = tagValue;
                neg.operatorName = "!";
                emit(std::move(neg));
            }
            br.trueLabel = thenLabel;
            br.falseLabel = endLabel;
            emit(std::move(br));

            current.blocks.push_back(BasicBlock{thenLabel, {}});
            currentBlock = current.blocks.size() - 1;
            pushScope();
            if (!match->bindingName.empty()) {
                const std::size_t payloadIndex = match->matchKind == "Ok" ? 1 : 2;
                auto payloadTypeNode = cloneTypeNode(match->left->checkedType->generics[match->matchKind == "Ok" ? 0 : 1].get());
                const Type payloadType = lowerType(payloadTypeNode.get());
                const std::string slot = declareLocal(match->bindingName);
                current.locals.emplace(slot, payloadType);
                Instruction extract;
                extract.op = Opcode::ExtractValue;
                extract.type = payloadType;
                extract.result = newTemp();
                extract.lhs = resultValue;
                extract.lhsType = exprType(match->left.get());
                extract.aggregateIndex = payloadIndex;
                const std::string extractedValue = extract.result;
                emit(std::move(extract));
                if (!TypeChecker::isCopyType(match->left->checkedType->generics[match->matchKind == "Ok" ? 0 : 1].get())) {
                    // Guard payloads of owning types are borrowed from the Result owner.
                    current.locals[slot] = Type{ScalarKind::Aggregate,0,false,"&borrow","ptr"};
                    Instruction addr; addr.op=Opcode::AggregateIndex; addr.type=current.locals[slot]; addr.result=newTemp(); addr.lhs=resultAddress; addr.sourceType=exprType(match->left.get()); addr.aggregateIndex=payloadIndex; addr.aggregateIndexConstant=true; emit(std::move(addr));
                    Instruction store; store.op=Opcode::StoreLocal; store.type=current.locals[slot]; store.slot=slot; store.lhs=addr.result; emit(std::move(store));
                    initializedSlots.insert(slot);
                } else {
                    Instruction store;
                    store.op = Opcode::StoreLocal;
                    store.type = payloadType;
                    store.slot = slot;
                    store.lhs = extractedValue;
                    emit(std::move(store));
                    initializedSlots.insert(slot);
                }
            }
            lowerStmt(stmt->guardBody.get());
            if (!terminated()) cleanupCurrentScope();
            popScope();
            if (!terminated()) ensureBranchTo(endLabel);

            current.blocks.push_back(BasicBlock{endLabel, {}});
            currentBlock = current.blocks.size() - 1;
            return;
        }
        case StmtKind::For: {
            const auto* iterableTypeRaw = stmt->iterable->checkedType.get();
            if (!iterableTypeRaw) unsupported("for iterable has no type", stmt->line);
            auto iterableType = cloneTypeNode(iterableTypeRaw);
            if (iterableType->isReference) iterableType->isReference = false;
            const bool fixed = iterableType->isArray && iterableType->fixedArraySize;
            const bool dynamic = iterableType->isArray && !iterableType->fixedArraySize;
            const bool view = iterableType->name == "View" || iterableType->name == "EditView";
            if (!fixed && !dynamic && !view) unsupported("for iterable is not a collection", stmt->line);
            std::string base;
            if (iterableTypeRaw->isReference) base = lowerExpr(stmt->iterable.get());
            else if (fixed) base = lowerLValueAddress(stmt->iterable.get());
            else {
                const auto addr = lowerLValueAddress(stmt->iterable.get());
                Instruction container; container.op=Opcode::LoadIndirect; container.type=lowerType(iterableType.get()); container.result=newTemp(); container.lhs=addr; emit(std::move(container));
                Instruction data; data.op=Opcode::ExtractValue; data.type=Type{ScalarKind::Aggregate,0,false,"ptr","ptr"}; data.result=newTemp(); data.lhs=container.result; data.lhsType=container.type; data.aggregateIndex=0; emit(std::move(data));
                base=data.result;
            }
            std::string length;
            if (fixed) length = std::to_string(*iterableType->fixedArraySize);
            else {
                const auto addr = iterableTypeRaw->isReference ? lowerExpr(stmt->iterable.get()) : lowerLValueAddress(stmt->iterable.get());
                Instruction container; container.op=Opcode::LoadIndirect; container.type=lowerType(iterableType.get()); container.result=newTemp(); container.lhs=addr; emit(std::move(container));
                Instruction len; len.op=Opcode::ExtractValue; len.type=Type{ScalarKind::Int,64,false,"usize","i64"}; len.result=newTemp(); len.lhs=container.result; len.lhsType=container.type; len.aggregateIndex=1; emit(std::move(len)); length=len.result;
                Instruction data; data.op=Opcode::ExtractValue; data.type=Type{ScalarKind::Aggregate,0,false,"ptr","ptr"}; data.result=newTemp(); data.lhs=container.result; data.lhsType=container.type; data.aggregateIndex=0; emit(std::move(data)); base=data.result;
            }
            if (fixed) {
                // fixed base is address of whole array; AggregateIndex handles the GEP.
            }
            const std::string idxSlot = "__lanner_for_idx." + std::to_string(++tempCounter);
            current.locals[idxSlot] = Type{ScalarKind::Int,64,false,"usize","i64"};
            localScopes.back()[idxSlot] = idxSlot;
            scopeOrder.back().push_back(idxSlot);
            initializedSlots.insert(idxSlot);
            Instruction zero; zero.op=Opcode::ConstInt; zero.type=current.locals[idxSlot]; zero.result=newTemp(); zero.intValue=0; emit(std::move(zero));
            Instruction zs; zs.op=Opcode::StoreLocal; zs.type=current.locals[idxSlot]; zs.slot=idxSlot; zs.lhs=zero.result; emit(std::move(zs));
            const auto condLabel=newBlockLabel("for_cond"); const auto bodyLabel=newBlockLabel("for_body"); const auto nextLabel=newBlockLabel("for_next"); const auto exitLabel=newBlockLabel("for_exit");
            ensureBranchTo(condLabel);
            current.blocks.push_back(BasicBlock{condLabel,{}}); currentBlock=current.blocks.size()-1;
            Instruction idx; idx.op=Opcode::LoadLocal; idx.type=current.locals[idxSlot]; idx.result=newTemp(); idx.slot=idxSlot; emit(std::move(idx));
            Instruction lim; lim.op=Opcode::ConstInt; lim.type=idx.type; lim.result=newTemp(); if (fixed) lim.intValue=*iterableType->fixedArraySize; emit(std::move(lim));
            if (!fixed) {
                // Recompute dynamic length each iteration from the original iterable address.
                const auto originalAddr = iterableTypeRaw->isReference ? lowerExpr(stmt->iterable.get()) : lowerLValueAddress(stmt->iterable.get());
                Instruction source; source.op=Opcode::LoadIndirect; source.type=lowerType(iterableType.get()); source.result=newTemp(); source.lhs=originalAddr; emit(std::move(source));
                Instruction dynlen; dynlen.op=Opcode::ExtractValue; dynlen.type=idx.type; dynlen.result=newTemp(); dynlen.lhs=source.result; dynlen.lhsType=source.type; dynlen.aggregateIndex=1; emit(std::move(dynlen)); lim.result=dynlen.result;
            }
            Instruction cmp; cmp.op=Opcode::Compare; cmp.type=Type{ScalarKind::Bool,1,false,"bool","i1"}; cmp.result=newTemp(); cmp.lhs=idx.result; cmp.rhs=lim.result; cmp.lhsType=idx.type; cmp.rhsType=lim.type; cmp.operatorName="<"; emit(std::move(cmp));
            Instruction br; br.op=Opcode::CondBranch; br.lhs=cmp.result; br.trueLabel=bodyLabel; br.falseLabel=exitLabel; emit(std::move(br));
            current.blocks.push_back(BasicBlock{bodyLabel,{}}); currentBlock=current.blocks.size()-1; pushScope(); loopScopeDepths.push_back(localScopes.size() - 1);
            Type elemType = fixed ? lowerType(arrayElementType(iterableType.get()).get()) :
                (view ? lowerType(iterableType->generics[0].get()) : lowerType(arrayElementType(iterableType.get()).get()));
            const std::string loopSlot=declareLocal(stmt->loopVar); current.locals[loopSlot]=elemType; initializedSlots.insert(loopSlot);
            Instruction idx2; idx2.op=Opcode::LoadLocal; idx2.type=current.locals[idxSlot]; idx2.result=newTemp(); idx2.slot=idxSlot; emit(std::move(idx2));
            Instruction ai; ai.op=Opcode::AggregateIndex; ai.type=elemType; ai.result=newTemp(); ai.lhs=base; ai.lhsType=Type{ScalarKind::Aggregate,0,false,"ptr","ptr"}; ai.rhs=idx2.result; ai.rhsType=idx2.type; ai.sourceType=fixed?lowerType(iterableType.get()):Type{ScalarKind::Aggregate,0,false,view?"viewbase":"dynbase","[1 x "+elemType.llvmName+"]"}; emit(std::move(ai));
            Instruction load; load.op=Opcode::LoadIndirect; load.type=elemType; load.result=newTemp(); load.lhs=ai.result; emit(std::move(load));
            Instruction st; st.op=Opcode::StoreLocal; st.type=elemType; st.slot=loopSlot; st.lhs=load.result; emit(std::move(st));
            lowerBlock(stmt->body); if (!terminated()) cleanupScopes(); popScope();
            if (!terminated()) ensureBranchTo(nextLabel);
            current.blocks.push_back(BasicBlock{nextLabel,{}}); currentBlock=current.blocks.size()-1;
            Instruction ix; ix.op=Opcode::LoadLocal; ix.type=current.locals[idxSlot]; ix.result=newTemp(); ix.slot=idxSlot; emit(std::move(ix)); Instruction one; one.op=Opcode::ConstInt; one.type=ix.type; one.result=newTemp(); one.intValue=1; emit(std::move(one)); Instruction add; add.op=Opcode::Binary; add.type=ix.type; add.result=newTemp(); add.lhs=ix.result; add.rhs=one.result; add.lhsType=ix.type; add.rhsType=one.type; add.operatorName="+"; emit(std::move(add)); Instruction store; store.op=Opcode::StoreLocal; store.type=ix.type; store.slot=idxSlot; store.lhs=add.result; emit(std::move(store)); ensureBranchTo(condLabel);
            current.blocks.push_back(BasicBlock{exitLabel,{}}); currentBlock=current.blocks.size()-1;
            return;
        }
        case StmtKind::ComptimeDecl: {
            SymbolTable localComptimeSymbols;
            for (const auto& [name, value] : comptimeGlobals) {
                Symbol symbol;
                symbol.name = name;
                symbol.isComptime = true;
                symbol.comptimeValue = value;
                symbol.declaredLine = stmt->line;
                localComptimeSymbols.declare(name, symbol);
            }
            for (const auto& scope : comptimeScopes) {
                localComptimeSymbols.pushScope();
                for (const auto& [name, value] : scope) {
                    Symbol symbol;
                    symbol.name = name;
                    symbol.isComptime = true;
                    symbol.comptimeValue = value;
                    symbol.declaredLine = stmt->line;
                    localComptimeSymbols.declare(name, symbol);
                }
            }
            ComptimeEvaluator evaluator(localComptimeSymbols);
            evaluator.evaluateAndDeclare(stmt);
            if (comptimeScopes.empty()) comptimeScopes.emplace_back();
            const Symbol* declared = localComptimeSymbols.resolve(stmt->comptimeName);
            if (!declared || !declared->comptimeValue) unsupported("failed to evaluate comptime declaration", stmt->line);
            comptimeScopes.back()[stmt->comptimeName] = *declared->comptimeValue;
            return;
        }
    }
}

[[noreturn]] void Lowerer::unsupported(const std::string& message, int line) const {
    throw std::runtime_error("HIR lowerer at line " + std::to_string(line) + ": " + message);
}

} // namespace lanner::hir
