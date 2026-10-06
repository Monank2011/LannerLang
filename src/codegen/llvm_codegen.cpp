#include "llvm_codegen.hpp"
#include "../sema/typechecker.hpp"
#include "../comptime/comptime_eval.hpp"
#include <charconv>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <locale>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {

int intBits(const std::string& name) {
    if (name == "usize" || name == "isize") return 64;
    if (name.size() < 2) return 0;
    try { return std::stoi(name.substr(1)); }
    catch (...) { return 0; }
}

bool typeLooksLikeInteger(const std::string& type) {
    return type.size() > 1 && type[0] == 'i' && std::isdigit(static_cast<unsigned char>(type[1])) != 0;
}

std::string hexByte(unsigned char value) {
    const char* digits = "0123456789ABCDEF";
    std::string out = "\\";
    out.push_back(digits[(value >> 4) & 0xF]);
    out.push_back(digits[value & 0xF]);
    return out;
}

int pointerBitsForTriple(const std::string& triple) {
    if (triple.empty()) return 64;
    const auto has = [&](const char* part) { return triple.find(part) != std::string::npos; };
    if (has("wasm32") || has("i386") || has("i486") || has("i586") || has("i686") ||
        has("armv7") || has("armv6") || has("thumbv7") || has("thumbv6") ||
        has("riscv32") || has("mipsel") || has("mips-") || has("powerpc-")) return 32;
    return 64;
}

bool isBuiltinExternDeclaration(const std::string& name) {
    return name == "malloc" || name == "realloc" || name == "free" ||
           name == "fopen" || name == "fseek" || name == "ftell" || name == "fread" ||
           name == "fclose" || name == "puts" || name == "fwrite" || name == "strlen" ||
           name == "getenv" || name == "printf" || name.rfind("__lanner_", 0) == 0 ||
           name.rfind("llvm.", 0) == 0;
}


} // namespace

bool LLVMCodeGenerator::isWebTarget() const { return targetTriple.rfind("wasm32", 0) == 0; }

bool LLVMCodeGenerator::isIntegerLLVM(const std::string& type) {
    return typeLooksLikeInteger(type);
}

bool LLVMCodeGenerator::isUnsignedType(const TypeNode* type) {
    return type && !type->name.empty() && (type->name[0] == 'u' || type->name == "usize");
}

bool LLVMCodeGenerator::isSignedType(const TypeNode* type) {
    return type && !type->name.empty() && (type->name[0] == 'i' || type->name == "isize");
}

std::unique_ptr<TypeNode> LLVMCodeGenerator::cloneType(const TypeNode* type) const {
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
    copy->isRawPointer = type->isRawPointer;
    copy->isUnsafeFunction = type->isUnsafeFunction;
    copy->origin = type->origin;
    copy->nestedOrigins = type->nestedOrigins;
    for (const auto& g : type->generics) copy->generics.push_back(cloneType(g.get()));
    return copy;
}

std::string LLVMCodeGenerator::llvmType(const TypeNode* type) const {
    if (!type) return "void";

    if (type->isReference || type->isRawPointer) return "ptr";

    if (type->isOptional) {
        auto payload = cloneType(type);
        payload->isOptional = false;
        return "{ i1, " + llvmType(payload.get()) + " }";
    }

    if (type->isArray) {
        if (type->fixedArraySize) {
            auto element = cloneType(type);
            element->isArray = false;
            element->fixedArraySize.reset();
            return "[" + std::to_string(*type->fixedArraySize) + " x " + llvmType(element.get()) + "]";
        }
        return "%LannerDynArray";
    }

    if (type->name == "void") return "void";
    if (type->name == "bool") return "i1";
    if (type->name == "Arena") return "%LannerArena";
    if (type->name == "Thread" || type->name == "Socket" || type->name == "Poller" ||
        type->name == "Mutex" || type->name == "RwLock" || type->name == "Condvar" ||
        type->name == "Semaphore" || type->name == "Process" || type->name == "Buffer" || type->name == "Tensor" || type->name == "GradTape" || type->name == "Regex" || type->name == "GameWindow" || type->name == "GameRenderer" || type->name == "GameTexture" || type->name == "GameAudio") return "ptr";
    if (type->name == "Atomic" && type->generics.size() == 1) return llvmType(type->generics[0].get());
    if (type->name == "v128") return "<2 x i64>";
    if (type->name == "v256") return "<4 x i64>";
    if (type->name == "v512") return "<8 x i64>";
    if ((type->name == "View" || type->name == "EditView") && type->generics.size() == 1) return "{ ptr, i64 }";
    if (type->name == "Result" && type->generics.size() == 2) {
        return "{ i1, " + llvmType(type->generics[0].get()) + ", " +
               llvmType(type->generics[1].get()) + " }";
    }
    if (type->name == "fn") return "ptr";
    if (type->name == "string") return "ptr";
    if (type->name == "f32") return "float";
    if (type->name == "f64") return "double";
    if (type->name == "usize" || type->name == "isize") return "i" + std::to_string(pointerBits);
    if ((type->name[0] == 'i' || type->name[0] == 'u') && intBits(type->name) >= 1 && intBits(type->name) <= 128) {
        return "i" + std::to_string(intBits(type->name));
    }
    if (enums.count(type->name)) return "i32";
    if (structs.count(type->name)) return "%" + type->name;

    unsupported("LLVM backend aggregate/nominal lowering is not yet implemented for '" + type->name + "'", 0);
}

std::string LLVMCodeGenerator::exprLLVMType(const Expr* expr) const {
    if (!expr) return "void";
    if (expr->checkedType) return llvmType(expr->checkedType.get());

    switch (expr->kind) {
        case ExprKind::IntLit: return "i32";
        case ExprKind::FloatLit: return "double";
        case ExprKind::BoolLit: return "i1";
        case ExprKind::StringLit: return "ptr";
        case ExprKind::NoneLit: return "ptr";
        case ExprKind::Identifier: {
            const auto* local = lookupLocal(expr->strValue);
            if (local) return llvmType(local->type);
            auto sit = statics.find(expr->strValue);
            if (sit != statics.end()) return llvmType(sit->second->type.get());
            unsupported("unknown value '" + expr->strValue + "'", expr->line);
        }
        case ExprKind::UnaryOp:
            if (expr->op == "!") return "i1";
            if (expr->op == "&" || expr->op == "&mut" || expr->op == "&raw") return "ptr";
            return exprLLVMType(expr->value.get());
        case ExprKind::BinaryOp:
            if (expr->op == "==" || expr->op == "!=" || expr->op == "<" || expr->op == ">" ||
                expr->op == "<=" || expr->op == ">=" || expr->op == "&&" || expr->op == "||") return "i1";
            return exprLLVMType(expr->left.get());
        case ExprKind::Call: {
            if (expr->callee->kind == ExprKind::FieldAccess &&
                expr->callee->target->kind == ExprKind::Identifier) {
                const auto ns = expr->callee->target->strValue;
                const auto member = expr->callee->field;
                if (ns == "Arena" && member == "create") return "%LannerArena";
                if (ns == "Game") {
                    if (member=="createWindow" || member=="createRenderer" || member=="createTexture" || member=="audioOpen") return "ptr";
                    if (member=="destroyWindow" || member=="requestClose" || member=="setTitle" || member=="present" || member=="destroyRenderer" || member=="presentRenderer" || member=="setDrawColor" || member=="clear" || member=="drawLine" || member=="fillRect" || member=="updateTexture" || member=="destroyTexture" || member=="audioPause" || member=="audioClose" || member=="sleepNanos") return member=="setDrawColor" || member=="clear" || member=="drawLine" || member=="fillRect" || member=="updateTexture" ? "i1" : "void";
                    if (member=="poll" || member=="width" || member=="height" || member=="eventType" || member=="eventCode" || member=="eventX" || member=="eventY") return "i32";
                    if (member=="shouldClose" || member=="setVSync" || member=="makeGLContext" || member=="keyDown" || member=="mouseButtonDown" || member=="controllerConnected") return "i1";
                    if (member=="windowFlags" || member=="rendererFlags") return "i32";
                    if (member=="eventText") return "ptr";
                    if (member=="mouseX" || member=="mouseY") return "i32";
                    if (member=="controllerAxis") return "float";
                    if (member=="controllerButtonDown") return "i1";
                    if (member=="audioWrite") return "i" + std::to_string(pointerBits);
                    if (member=="audioQueued") return "i" + std::to_string(pointerBits);
                    if (member=="timeNanos") return "i64";
                    if (member=="deltaSeconds") return "double";
                }
                if (ns == "Graphics") {
                    if (member=="available") return "i1";
                    if (member=="backend") return "ptr";
                    if (member=="loadProc") return "ptr";
                    if (member=="glCreateShader" || member=="glCreateProgram") return "i32";
                    if (member=="glShaderStatus" || member=="glProgramStatus") return "i1";
                    if (member=="glShaderLog" || member=="glProgramLog") return "ptr";
                    if (member=="glGetError") return "i32";
                    return "void";
                }
                if (ns == "Mobile") {
                    if (member == "log") return "void";
                    if (member == "platform" || member == "osVersion" || member == "appDataPath" || member == "documentsPath" || member == "cachePath") return "ptr";
                    if (member == "isSimulator" || member == "cameraAvailable" || member == "locationAvailable" || member == "bluetoothAvailable") return "i1";
                    if (member == "screenWidth" || member == "screenHeight" || member == "safeAreaTop" || member == "safeAreaBottom" || member == "safeAreaLeft" || member == "safeAreaRight") return "i32";
                    if (member == "deviceScale") return "double";
                    if (member == "openUrl" || member == "clipboardSet" || member == "vibrate") return "i1";
                    if (member == "requestPermission") return "i32";
                    if (member == "clipboardGet") return "ptr";
                }
                if (ns == "Web") {
                    if (member == "log" || member == "warn" || member == "error" ||
                        member == "clearTimeout" || member == "cancelAnimationFrame" ||
                        member == "removeEventListener" || member == "freeBuffer") return "void";
                    if (member == "nowMs" || member == "random") return "double";
                    if (member == "setText" || member == "setHtml" || member == "setAttribute" ||
                        member == "addClass" || member == "removeClass" || member == "remove" ||
                        member == "queryCount" || member == "focus" || member == "setTimeout" ||
                        member == "requestAnimationFrame" || member == "queueMicrotask" ||
                        member == "addEventListener" || member == "fetchText") return "i32";
                }
                if (ns == "Args" && member == "count") return "i64";
                if (ns == "Args" && member == "at") return "ptr";
                if (ns == "Env" && member == "get") return "ptr";
                if (ns == "Env" && (member == "has" || member == "set" || member == "unset")) return "i1";
                if (ns == "FS" && (member == "exists" || member == "isFile" || member == "isDir" || member == "write" || member == "append" || member == "writeBuffer" || member == "remove" || member == "mkdir" || member == "rmdir" || member == "rename" || member == "copy" || member == "chdir")) return "i1";
                if (ns == "FS" && member == "fileSize") return "i64";
                if (ns == "FS" && (member == "read" || member == "list")) return "ptr";
                if (ns == "FS" && member == "cwd") return "ptr";
                if (ns == "Path" && (member == "join" || member == "basename" || member == "dirname" || member == "extension" || member == "stem" || member == "normalize" || member == "absolute")) return "ptr";
                if (ns == "Path" && member == "isAbsolute") return "i1";
                if (ns == "Regex" && member == "compile") return "ptr";
                if (ns == "Regex" && member == "isMatch") return "i1";
                if (ns == "Regex" && member == "find") return "i64";
                if (ns == "Regex" && member == "free") return "void";
                if (ns == "Shell" && member == "run") return "i32";
                if (ns == "Shell" && member == "output") return "ptr";
                if (ns == "Shell" && member == "which") return "ptr";
                if (ns == "String" && member == "parseInt") return "i64";
                if (ns == "String" && member == "parseFloat") return "double";
                if (ns == "Stdin" && member == "hasInput") return "i1";
                if (ns == "Stdin" && member == "readLine") return "ptr";
                if (ns == "Clock" && (member == "monotonicNanos" || member == "deadlineAfterNanos" || member == "remainingNanos")) return "i64";
                if (ns == "Clock" && member == "expired") return "i1";
                if (ns == "Clock" && member == "sleepNanos") return "void";
                if (ns == "Thread" && member == "spawn") return "ptr";
                if (ns == "Thread" && member == "hardwareConcurrency") return "i64";
                if (ns == "Thread" && member == "yield") return "void";
                if (ns == "Net") {
                    if (member == "tcpConnect" || member == "tcpListen" || member == "accept" || member == "udpOpen") return "ptr";
                    if (member == "close") return "void";
                    if (member == "send" || member == "recv" || member == "sendString" || member == "udpSendTo" || member == "udpRecv") return "i" + std::to_string(pointerBits);
                    if (member == "setNonblocking" || member == "tcpNoDelay") return "i1";
                    if (member == "poll" || member == "shutdown" || member == "lastError") return "i32";
                    if (member == "localPort" || member == "peerPort") return "i16";
                    if (member == "errorString") return "ptr";
                    if (member == "udpBind") return "i1";
                }
                if (ns == "Poller") {
                    if (member == "create" || member == "eventSocket") return "ptr";
                    if (member == "destroy") return "void";
                    if (member == "add" || member == "remove") return "i1";
                    if (member == "wait" || member == "count" || member == "eventMask" || member == "readEvents" || member == "writeEvents" || member == "errorEvents") return "i32";
                }
                if (ns == "Mutex") {
                    if (member == "create") return "ptr";
                    if (member == "lock" || member == "unlock" || member == "destroy") return "void";
                    if (member == "tryLock") return "i1";
                }
                if (ns == "RwLock") {
                    if (member == "create") return "ptr";
                    if (member == "readLock" || member == "writeLock" || member == "unlock" || member == "destroy") return "void";
                    if (member == "tryReadLock" || member == "tryWriteLock") return "i1";
                }
                if (ns == "Condvar") {
                    if (member == "create") return "ptr";
                    if (member == "wait") return "i1";
                    if (member == "signal" || member == "broadcast" || member == "destroy") return "void";
                }
                if (ns == "Semaphore") {
                    if (member == "create") return "ptr";
                    if (member == "wait" || member == "tryWait") return "i1";
                    if (member == "post" || member == "destroy") return "void";
                }
                if (ns == "Buffer") {
                    if (member == "new" || member == "fromString") return "ptr";
                }
        if (ns == "Process") {
                    if (member == "run" || member == "wait") return "i32";
                    if (member == "spawn" || member == "output") return "ptr";
                    if (member == "argCount") return "i64";
                    if (member == "arg") return "ptr";
                    if (member == "setEnv") return "i1";
                }
                if (ns == "Http") {
                    if (member == "get") return "ptr";
                    if (member == "status") return "i32";
                }
                if (ns == "Tensor") {
                    if (member == "zeros1" || member == "zeros2" || member == "zeros3" || member == "zeros4" || member == "ones1" || member == "ones2" || member == "ones3" || member == "ones4" || member == "zerosF32" || member == "onesF32" || member == "zerosF64" || member == "onesF64" || member == "from1F32" || member == "from1F64" || member == "from2F32" || member == "from2F64" || member == "clone" || member == "add" || member == "sub" || member == "mul" || member == "div" || member == "scale" || member == "relu" || member == "sigmoid" || member == "tanh" || member == "softmax" || member == "matmul" || member == "reshape2" || member == "reshape3" || member == "reshape4" || member == "transpose2" || member == "slice" || member == "contiguous" || member == "conv2d") return "ptr";
                    if (member == "free" || member == "fill") return "void";
                    if (member == "rank" || member == "len" || member == "dim" || member == "stride") return "i64";
                    if (member == "dtype") return "i32";
                    if (member == "isContiguous") return "i1";
                    if (member == "dataF32") return "ptr";
                    if (member == "dataF64") return "ptr";
                    if (member == "get1" || member == "get2" || member == "get3" || member == "sum" || member == "mean" || member == "l2Norm" || member == "dot") return "double";
                    if (member == "set1" || member == "set2" || member == "set3" || member == "argmax") return member == "argmax" ? "i64" : "void";
                }
                if (ns == "Grad") {
                    if (member == "create") return "ptr";
                    if (member == "watch" || member == "backward" || member == "free") return "void";
                    if (member == "add" || member == "mul" || member == "matmul" || member == "relu" || member == "tanh" || member == "sum" || member == "scale" || member == "grad") return "ptr";
                }
                if (ns == "Accel") {
                    if (member == "cudaAvailable" || member == "rocmAvailable" || member == "metalAvailable" || member == "blasAvailable") return "i1";
                    if (member == "backend") return "ptr";
                }
                if (ns == "Json") {
                    if (member == "validate") return "i1";
                    if (member == "quote" || member == "int" || member == "float" || member == "bool" || member == "nullValue") return "ptr";
                }
                if (ns == "Cpu" && (member == "hasAvx2" || member == "hasAvx512" || member == "hasSse42" || member == "hasBmi2" || member == "hasPopcnt")) return "i1";
                if (ns == "Cpu" && (member == "rdtsc" || member == "pext" || member == "pdep")) return "i64";
                if (ns == "Cpu" && (member == "popcount" || member == "ctz" || member == "clz")) return "i32";
                if (ns == "Cpu" && member == "bswap") return "i64";
                if (ns == "Cpu" && member == "loadV128") return "<2 x i64>";
                if (ns == "Cpu" && member == "loadV256") return "<4 x i64>";
                if (ns == "Cpu" && member == "loadV512") return "<8 x i64>";
                if (ns == "Cpu" && (member == "zeroV128" || member == "zeroV256" || member == "zeroV512")) return member == "zeroV128" ? "<2 x i64>" : member == "zeroV256" ? "<4 x i64>" : "<8 x i64>";
            }
            if (expr->callee->kind == ExprKind::FieldAccess && expr->callee->target && expr->callee->target->checkedType && expr->callee->target->checkedType->name == "GradTape") {
                if (expr->callee->field == "free") return "void";
            }
            if (expr->callee->kind == ExprKind::FieldAccess && expr->callee->target && expr->callee->target->checkedType && expr->callee->target->checkedType->name == "Tensor") {
                const auto member = expr->callee->field;
                if (member == "clone" || member == "add" || member == "sub" || member == "mul" || member == "div" || member == "scale" || member == "relu" || member == "sigmoid" || member == "tanh" || member == "softmax" || member == "matmul" || member == "reshape2" || member == "reshape3" || member == "reshape4" || member == "transpose2" || member == "slice" || member == "contiguous" || member == "conv2d") return "ptr";
                if (member == "free" || member == "set1" || member == "set2" || member == "set3" || member == "fill") return "void";
                if (member == "rank" || member == "len" || member == "dim" || member == "stride" || member == "argmax") return "i" + std::to_string(pointerBits);
                if (member == "dtype") return "i32";
                if (member == "isContiguous") return "i1";
                if (member == "dataF32" || member == "dataF64") return "ptr";
                if (member == "get1" || member == "get2" || member == "get3" || member == "sum" || member == "mean" || member == "l2Norm" || member == "dot") return "double";
            }
            if (expr->callee->kind == ExprKind::FieldAccess && expr->callee->target && expr->callee->target->checkedType && expr->callee->target->checkedType->name == "Buffer") {
                const auto member = expr->callee->field;
                if (member == "len") return "i" + std::to_string(pointerBits);
                if (member == "data") return "ptr";
                if (member == "cstr") return "ptr";
                if (member == "free" || member == "appendString" || member == "appendBuffer") return member == "free" ? "void" : "i1";
            }
            if (expr->callee->kind == ExprKind::FieldAccess && expr->callee->target && expr->callee->target->checkedType && expr->callee->target->checkedType->isRawPointer) {
                const auto member = expr->callee->field;
                if (member == "add" || member == "sub" || member == "offset") return "ptr";
            }
            if (expr->callee->kind != ExprKind::Identifier) unsupported("only direct function calls and built-in methods are lowered", expr->line);
            const auto& builtin = expr->callee->strValue;
            if (builtin == "readFile") return "%LannerDynArray";
            if (builtin == "writeStdout" || builtin == "writeRaw" || builtin == "writeIntRaw" ||
                builtin == "writeByteRaw" || builtin == "print" || builtin == "printInt") return "void";
            if (builtin == "stringLen") return "i64";
            if (builtin == "getEnv") return "ptr";
            if (builtin == "alloc" || builtin == "realloc" || builtin == "allocAligned" || builtin == "stackAlloc" || builtin == "memcpy" || builtin == "memset" || builtin == "memmove") return "ptr";
            if (builtin == "sizeOf" || builtin == "alignOf" || builtin == "offsetOf" || builtin == "ptrDiff") return "i" + std::to_string(pointerBits);
            if (builtin == "dealloc" || builtin == "deallocAligned" || builtin == "volatileStore" || builtin == "unalignedStore" || builtin == "asm" || builtin == "atomicFence" || builtin == "compilerFence" || builtin == "unreachable" || builtin == "assume" || builtin == "trap") return "void";
            if (builtin == "memcmp") return "i32";
            if (builtin == "unalignedLoad") {
                if (expr->args.empty() || !expr->args[0]->checkedType || !expr->args[0]->checkedType->isRawPointer) return "i8";
                auto pointee = cloneType(expr->args[0]->checkedType.get()); pointee->isRawPointer=false; pointee->isMutable=false; return llvmType(pointee.get());
            }
            if (builtin == "volatileLoad") {
                if (expr->args.empty() || !expr->args[0]->checkedType || !expr->args[0]->checkedType->isRawPointer) return "i8";
                auto pointee = cloneType(expr->args[0]->checkedType.get());
                pointee->isRawPointer = false; pointee->isMutable = false;
                return llvmType(pointee.get());
            }
            if (builtin == "asmI64") return "i64";
            if (builtin == "asmI32") return "i32";
            if (builtin == "asmPtr") return "ptr";
            auto it = functions.find(expr->callee->strValue);
            if (it == functions.end()) unsupported("unknown function '" + expr->callee->strValue + "'", expr->line);
            return llvmType(it->second->returnType.get());
        }
        case ExprKind::FieldAccess:
            if (expr->target->kind == ExprKind::Identifier && enums.count(expr->target->strValue)) return "i32";
            unsupported("field access is not yet lowered for this type", expr->line);
        case ExprKind::Index:
            if (expr->checkedType) return llvmType(expr->checkedType.get());
            unsupported("index expression is not yet lowered", expr->line);
        case ExprKind::Slice:
            if (expr->checkedType) return llvmType(expr->checkedType.get());
            return "{ ptr, i64 }";
        case ExprKind::Cast: return llvmType(expr->castType.get());
        default: unsupported("expression is not a scalar LLVM value", expr->line);
    }
}

std::string LLVMCodeGenerator::newTemp(const std::string& prefix) {
    return "%" + prefix + std::to_string(tempCounter++);
}

std::string LLVMCodeGenerator::newLabel(const std::string& prefix) {
    return prefix + std::to_string(labelCounter++);
}

[[noreturn]] void LLVMCodeGenerator::unsupported(const std::string& message, int line) const {
    throw std::runtime_error("LLVM codegen error" + (line ? " at line " + std::to_string(line) : "") + ": " + message);
}

void LLVMCodeGenerator::pushScope() {
    localScopes.emplace_back();
    scopeOrder.emplace_back();
    comptimeScopes.emplace_back();
}
void LLVMCodeGenerator::popScope() {
    if (!localScopes.empty()) localScopes.pop_back();
    if (!scopeOrder.empty()) scopeOrder.pop_back();
    if (!comptimeScopes.empty()) comptimeScopes.pop_back();
}

const ComptimeValue* LLVMCodeGenerator::lookupComptime(const std::string& name) const {
    for (auto it = comptimeScopes.rbegin(); it != comptimeScopes.rend(); ++it) {
        const auto found = it->find(name);
        if (found != it->end()) return &found->second;
    }
    const auto global = comptimeGlobals.find(name);
    return global == comptimeGlobals.end() ? nullptr : &global->second;
}

const LLVMCodeGenerator::LocalBinding* LLVMCodeGenerator::lookupLocal(const std::string& name) const {
    for (auto it = localScopes.rbegin(); it != localScopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return &found->second;
    }
    return nullptr;
}

LLVMCodeGenerator::LocalBinding* LLVMCodeGenerator::lookupLocal(const std::string& name) {
    for (auto it = localScopes.rbegin(); it != localScopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return &found->second;
    }
    return nullptr;
}

std::string LLVMCodeGenerator::comptimeLiteral(const ComptimeValue& value) {
    if (std::holds_alternative<std::int64_t>(value)) return std::to_string(std::get<std::int64_t>(value));
    if (std::holds_alternative<double>(value)) return std::to_string(std::get<double>(value));
    if (std::holds_alternative<bool>(value)) return std::get<bool>(value) ? "1" : "0";
    return llvmStringLiteral(std::get<std::string>(value));
}

std::string LLVMCodeGenerator::llvmFloatLiteral(const Expr* expr) const {
    if (!expr || expr->kind != ExprKind::FloatLit) {
        unsupported("invalid floating-point literal expression", expr ? expr->line : 0);
    }

    const double parsed = std::stod(expr->strValue);
    const bool isF32 = expr->checkedType && expr->checkedType->name == "f32";

    // Emit LLVM's exact IEEE-754 legacy hexadecimal bit encoding.  This is
    // intentionally preferred over decimal spelling because LLVM requires a
    // decimal FP literal to be exactly representable in the destination type;
    // ordinary source literals such as 0.9 are not.  The 16-digit legacy form
    // is accepted by the modern opaque-pointer LLVM toolchain as well as older
    // LLVM releases, making it the most portable textual representation while
    // keeping the source-level f32/f64 value exact.
    const double canonical = isF32 ? static_cast<double>(static_cast<float>(parsed)) : parsed;
    std::uint64_t bits = 0;
    static_assert(sizeof(bits) == sizeof(canonical), "unexpected double size");
    std::memcpy(&bits, &canonical, sizeof(bits));

    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << std::setw(16) << std::setfill('0') << bits;
    return out.str();
}

std::string LLVMCodeGenerator::normalizeIntLiteral(const std::string& literal) const {
    int base = 10;
    std::size_t start = 0;
    if (literal.size() > 2 && literal[0] == '0' && (literal[1] == 'x' || literal[1] == 'X')) {
        base = 16;
        start = 2;
    } else if (literal.size() > 2 && literal[0] == '0' && (literal[1] == 'b' || literal[1] == 'B')) {
        base = 2;
        start = 2;
    } else {
        return literal;
    }

    std::uint64_t value = 0;
    const auto result = std::from_chars(literal.data() + start, literal.data() + literal.size(), value, base);
    if (result.ec != std::errc{} || result.ptr != literal.data() + literal.size()) {
        unsupported("invalid integer literal '" + literal + "'", 0);
    }
    return std::to_string(value);
}

std::string LLVMCodeGenerator::normalizeIndexToI64(const Expr* expr, const std::string& value) {
    const std::string from = exprLLVMType(expr);
    if (from == "i64") return value;
    if (!isIntegerLLVM(from)) unsupported("array index must lower to an integer", expr ? expr->line : 0);
    const auto temp = newTemp("idx64");
    const int fromBits = std::stoi(from.substr(1));
    if (fromBits < 64) {
        // Signed indices are sign-extended. Negative values will therefore fail
        // the unsigned bounds check below instead of wrapping into the array.
        const bool signedIndex = expr->checkedType && isSignedType(expr->checkedType.get());
        body.push_back("  " + temp + " = " + (signedIndex ? "sext" : "zext") + " " + from + " " + value + " to i64");
    } else if (fromBits > 64) {
        unsupported("array index wider than 64 bits is not supported", expr ? expr->line : 0);
    }
    return temp;
}

void LLVMCodeGenerator::emitBoundsCheck(const Expr* index, const std::string& indexI64, std::uint64_t size) {
    const auto ok = newLabel("bounds.ok");
    const auto bad = newLabel("bounds.trap");
    const auto check = newTemp("bounds");
    body.push_back("  " + check + " = icmp ult i64 " + indexI64 + ", " + std::to_string(size));
    body.push_back("  br i1 " + check + ", label %" + ok + ", label %" + bad);
    body.push_back(bad + ":");
    body.push_back("  call void @llvm.trap()");
    body.push_back("  unreachable");
    body.push_back(ok + ":");
    (void)index;
}

void LLVMCodeGenerator::emitDynamicBoundsCheck(const std::string& indexI64, const std::string& lengthI64) {
    const auto ok = newLabel("bounds.ok");
    const auto bad = newLabel("bounds.trap");
    const auto check = newTemp("bounds");
    body.push_back("  " + check + " = icmp ult i64 " + indexI64 + ", " + lengthI64);
    body.push_back("  br i1 " + check + ", label %" + ok + ", label %" + bad);
    body.push_back(bad + ":");
    body.push_back("  call void @llvm.trap()");
    body.push_back("  unreachable");
    body.push_back(ok + ":");
}


std::string LLVMCodeGenerator::dynamicArrayElementType(const TypeNode* arrayType) const {
    if (!arrayType || !arrayType->isArray || arrayType->fixedArraySize) unsupported("expected a dynamic array type", 0);
    auto element = cloneType(arrayType);
    element->isArray = false;
    element->fixedArraySize.reset();
    element->origin = {};
    return llvmType(element.get());
}

std::string LLVMCodeGenerator::emitTypeSize(const std::string& elementType) {
    // LLVM constant-expression GEP gives the target-dependent byte size without
    // hard-coding a platform data model into the front-end.
    return "ptrtoint (ptr getelementptr (" + elementType + ", ptr null, i64 1) to i64)";
}

std::string LLVMCodeGenerator::emitTypeAlign(const std::string& elementType) {
    (void)elementType;
    return "8";
}

std::string LLVMCodeGenerator::arenaAddressFor(const std::string& arenaName, int line) {
    auto* arena = lookupLocal(arenaName);
    if (!arena || !arena->type || arena->type->name != "Arena") {
        unsupported("unknown Arena '" + arenaName + "'", line);
    }
    return arena->addr;
}

void LLVMCodeGenerator::cleanupBinding(const std::string&, LocalBinding& local) {
    if (local.moved || local.cleaned || !local.type || local.type->isReference) return;

    // Destruction is recursive because LANNER aggregates can own arrays, Results,
    // optionals, fixed arrays, and other structs. Every path ends at the same
    // textual continuation block, so subsequent cleanup remains well-formed LLVM.
    std::function<void(const TypeNode*, const std::string&)> cleanupAt;
    cleanupAt = [&](const TypeNode* type, const std::string& address) {
        if (!type || type->isReference || TypeChecker::isCopyType(type)) return;

        if (type->isOptional) {
            auto payloadType = cloneType(type);
            payloadType->isOptional = false;
            const auto value = newTemp("cleanup.optional");
            body.push_back("  " + value + " = load " + llvmType(type) + ", ptr " + address);
            const auto tag = newTemp("cleanup.optional.tag");
            body.push_back("  " + tag + " = extractvalue " + llvmType(type) + " " + value + ", 0");
            const auto active = newLabel("cleanup.optional.active");
            const auto done = newLabel("cleanup.optional.done");
            body.push_back("  br i1 " + tag + ", label %" + active + ", label %" + done);
            body.push_back(active + ":");
            const auto payload = newTemp("cleanup.optional.payload");
            body.push_back("  " + payload + " = extractvalue " + llvmType(type) + " " + value + ", 1");
            const auto payloadAddr = newTemp("cleanup.optional.addr");
            body.push_back("  " + payloadAddr + " = alloca " + llvmType(payloadType.get()));
            body.push_back("  store " + llvmType(payloadType.get()) + " " + payload + ", ptr " + payloadAddr);
            cleanupAt(payloadType.get(), payloadAddr);
            body.push_back("  br label %" + done);
            body.push_back(done + ":");
            return;
        }

        if (type->name == "Result" && type->generics.size() == 2) {
            const auto resultType = llvmType(type);
            const auto value = newTemp("cleanup.result");
            body.push_back("  " + value + " = load " + resultType + ", ptr " + address);
            const auto tag = newTemp("cleanup.result.tag");
            body.push_back("  " + tag + " = extractvalue " + resultType + " " + value + ", 0");
            const auto okLabel = newLabel("cleanup.result.ok");
            const auto errLabel = newLabel("cleanup.result.err");
            const auto done = newLabel("cleanup.result.done");
            body.push_back("  br i1 " + tag + ", label %" + okLabel + ", label %" + errLabel);

            body.push_back(okLabel + ":");
            const auto okPayload = newTemp("cleanup.result.ok.payload");
            body.push_back("  " + okPayload + " = extractvalue " + resultType + " " + value + ", 1");
            const auto okAddr = newTemp("cleanup.result.ok.addr");
            body.push_back("  " + okAddr + " = alloca " + llvmType(type->generics[0].get()));
            body.push_back("  store " + llvmType(type->generics[0].get()) + " " + okPayload + ", ptr " + okAddr);
            cleanupAt(type->generics[0].get(), okAddr);
            body.push_back("  br label %" + done);

            body.push_back(errLabel + ":");
            const auto errPayload = newTemp("cleanup.result.err.payload");
            body.push_back("  " + errPayload + " = extractvalue " + resultType + " " + value + ", 2");
            const auto errAddr = newTemp("cleanup.result.err.addr");
            body.push_back("  " + errAddr + " = alloca " + llvmType(type->generics[1].get()));
            body.push_back("  store " + llvmType(type->generics[1].get()) + " " + errPayload + ", ptr " + errAddr);
            cleanupAt(type->generics[1].get(), errAddr);
            body.push_back("  br label %" + done);
            body.push_back(done + ":");
            return;
        }

        if (type->isArray && !type->fixedArraySize) {
            const auto array = newTemp("cleanup.array");
            body.push_back("  " + array + " = load %LannerDynArray, ptr " + address);
            const auto data = newTemp("cleanup.data");
            const auto len = newTemp("cleanup.len");
            const auto arena = newTemp("cleanup.arena");
            body.push_back("  " + data + " = extractvalue %LannerDynArray " + array + ", 0");
            body.push_back("  " + len + " = extractvalue %LannerDynArray " + array + ", 1");
            body.push_back("  " + arena + " = extractvalue %LannerDynArray " + array + ", 3");

            auto elementType = cloneType(type);
            elementType->isArray = false;
            elementType->fixedArraySize.reset();
            elementType->origin = {};
            if (!TypeChecker::isCopyType(elementType.get())) {
                const auto indexAddr = newTemp("cleanup.index.addr");
                body.push_back("  " + indexAddr + " = alloca i64");
                body.push_back("  store i64 0, ptr " + indexAddr);
                const auto loop = newLabel("cleanup.array.loop");
                const auto afterLoop = newLabel("cleanup.array.after");
                body.push_back("  br label %" + loop);
                body.push_back(loop + ":");
                const auto index = newTemp("cleanup.index");
                body.push_back("  " + index + " = load i64, ptr " + indexAddr);
                const auto has = newTemp("cleanup.has");
                body.push_back("  " + has + " = icmp ult i64 " + index + ", " + len);
                const auto element = newLabel("cleanup.array.element");
                body.push_back("  br i1 " + has + ", label %" + element + ", label %" + afterLoop);
                body.push_back(element + ":");
                const auto elemAddr = newTemp("cleanup.elem.addr");
                body.push_back("  " + elemAddr + " = getelementptr " + llvmType(elementType.get()) + ", ptr " + data + ", i64 " + index);
                cleanupAt(elementType.get(), elemAddr);
                const auto next = newTemp("cleanup.next");
                body.push_back("  " + next + " = add i64 " + index + ", 1");
                body.push_back("  store i64 " + next + ", ptr " + indexAddr);
                body.push_back("  br label %" + loop);
                body.push_back(afterLoop + ":");
            }

            const auto owned = newTemp("cleanup.owned");
            body.push_back("  " + owned + " = icmp eq ptr " + arena + ", null");
            const auto doFree = newLabel("cleanup.free");
            const auto done = newLabel("cleanup.array.done");
            body.push_back("  br i1 " + owned + ", label %" + doFree + ", label %" + done);
            body.push_back(doFree + ":");
            const auto hasData = newTemp("cleanup.hasdata");
            body.push_back("  " + hasData + " = icmp ne ptr " + data + ", null");
            const auto freeIt = newLabel("cleanup.free.data");
            const auto skip = newLabel("cleanup.skip.data");
            body.push_back("  br i1 " + hasData + ", label %" + freeIt + ", label %" + skip);
            body.push_back(freeIt + ":");
            body.push_back("  call void @free(ptr " + data + ")");
            body.push_back("  br label %" + skip);
            body.push_back(skip + ":");
            body.push_back("  br label %" + done);
            body.push_back(done + ":");
            return;
        }

        if (type->fixedArraySize) {
            for (std::uint64_t i = 0; i < *type->fixedArraySize; ++i) {
                auto elementType = cloneType(type);
                elementType->isArray = false;
                elementType->fixedArraySize.reset();
                const auto elemAddr = newTemp("cleanup.fixed.elem");
                body.push_back("  " + elemAddr + " = getelementptr " + llvmType(type) + ", ptr " + address + ", i64 0, i64 " + std::to_string(i));
                cleanupAt(elementType.get(), elemAddr);
            }
            return;
        }

        if (structs.count(type->name)) {
            const auto* st = structs.at(type->name);
            for (std::size_t i = 0; i < st->fields.size(); ++i) {
                const auto fieldAddr = newTemp("cleanup.struct.field");
                body.push_back("  " + fieldAddr + " = getelementptr " + llvmType(type) + ", ptr " + address + ", i32 0, i32 " + std::to_string(i));
                cleanupAt(st->fields[i].type.get(), fieldAddr);
            }
            return;
        }

        if (type->name == "Arena") {
            body.push_back("  call void @__lanner_arena_destroy(ptr " + address + ")");
        }
    };

    cleanupAt(local.type, local.addr);
    local.cleaned = true;
}

void LLVMCodeGenerator::cleanupCurrentScope() {
    if (localScopes.empty()) return;
    const auto& order = scopeOrder.back();
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        auto found = localScopes.back().find(*it);
        if (found != localScopes.back().end()) cleanupBinding(*it, found->second);
    }
}

void LLVMCodeGenerator::cleanupScopesFrom(std::size_t depth) {
    if (depth > localScopes.size()) return;
    for (std::size_t i = localScopes.size(); i-- > depth;) {
        for (auto it = scopeOrder[i].rbegin(); it != scopeOrder[i].rend(); ++it) {
            auto found = localScopes[i].find(*it);
            if (found != localScopes[i].end()) cleanupBinding(*it, found->second);
        }
    }
}

void LLVMCodeGenerator::cleanupAllScopes() { cleanupScopesFrom(0); }

void LLVMCodeGenerator::markMovedArgument(const Expr* arg, const TypeNode* paramType) {
    if (!arg || arg->kind != ExprKind::Identifier || !arg->checkedType) return;
    // Passing an identifier into a &T/&mut T (or View/EditView) parameter borrows
    // it for the call only — ownership stays with the caller, so the caller's
    // local must NOT be flagged as moved just because it was named as an argument.
    // This mirrors the (correct) rule the typechecker itself already applies for
    // the same call site; this backend previously lacked the reference/view guard,
    // which produced false "use of moved local" errors for reference parameters
    // reused across independent branches.
    if (paramType) {
        const bool isViewType = !paramType->isArray && !paramType->isReference && !paramType->isOptional &&
                                 (paramType->name == "View" || paramType->name == "EditView") &&
                                 paramType->generics.size() == 1;
        if (paramType->isReference || isViewType) return;
    }
    auto* local = lookupLocal(arg->strValue);
    if (!local) return;
    if (!TypeChecker::isCopyType(arg->checkedType.get())) local->moved = true;
}

std::string LLVMCodeGenerator::emitArenaCreate(const Expr* expr) {
    if (expr->args.size() != 1) unsupported("Arena.create() takes one argument", expr->line);
    const auto size = emitExpr(expr->args[0].get());
    const auto init = newTemp("arena.init");
    const auto normalized = newTemp("arena.size");
    const auto sizeType = exprLLVMType(expr->args[0].get());
    if (sizeType != "i64") {
        const bool signedSource = expr->args[0]->checkedType && isSignedType(expr->args[0]->checkedType.get());
        body.push_back("  " + normalized + " = " + (signedSource ? "sext" : "zext") + " " + sizeType + " " + size + " to i64");
    } else {
        body.push_back("  " + normalized + " = add i64 " + size + ", 0");
    }
    // A newly created arena starts empty. The requested size is the initial chunk size.
    // Actual memory is obtained lazily on the first allocation.
    const auto a0 = newTemp("arena.a0");
    const auto a1 = newTemp("arena.a1");
    const auto a2 = newTemp("arena.a2");
    body.push_back("  " + a0 + " = insertvalue %LannerArena zeroinitializer, i64 " + normalized + ", 3");
    body.push_back("  " + a1 + " = insertvalue %LannerArena " + a0 + ", ptr null, 0");
    body.push_back("  " + a2 + " = insertvalue %LannerArena " + a1 + ", ptr null, 1");
    body.push_back("  " + init + " = insertvalue %LannerArena " + a2 + ", i64 0, 2");
    return init;
}

std::string LLVMCodeGenerator::emitDynamicArrayPush(const Expr* target, const Expr* value, int line) {
    const auto* checkedTarget = target ? target->checkedType.get() : nullptr;
    if (!checkedTarget) unsupported("push target has no type annotation", line);
    auto targetType = cloneType(checkedTarget);
    if (targetType->isReference) targetType->isReference = false;
    if (!targetType->isArray || targetType->fixedArraySize) unsupported("push target is not a dynamic array", line);

    std::string targetAddr;
    if (checkedTarget->isReference) {
        targetAddr = emitExpr(target);
    } else {
        targetAddr = emitLValueAddress(target);
    }

    auto elem = cloneType(targetType.get());
    elem->isArray = false;
    elem->fixedArraySize.reset();
    elem->origin = {};
    elem->isReference = false;
    elem->isMutable = false;
    const auto elemType = llvmType(elem.get());
    const auto valueIR = emitExpr(value);
    const auto array = newTemp("array.load");
    body.push_back("  " + array + " = load %LannerDynArray, ptr " + targetAddr);
    const auto data = newTemp("array.data");
    const auto len = newTemp("array.len");
    const auto cap = newTemp("array.cap");
    const auto arena = newTemp("array.arena");
    body.push_back("  " + data + " = extractvalue %LannerDynArray " + array + ", 0");
    body.push_back("  " + len + " = extractvalue %LannerDynArray " + array + ", 1");
    body.push_back("  " + cap + " = extractvalue %LannerDynArray " + array + ", 2");
    body.push_back("  " + arena + " = extractvalue %LannerDynArray " + array + ", 3");

    const auto full = newTemp("array.full");
    body.push_back("  " + full + " = icmp uge i64 " + len + ", " + cap);
    const auto grow = newLabel("array.grow");
    const auto write = newLabel("array.write");
    body.push_back("  br i1 " + full + ", label %" + grow + ", label %" + write);

    body.push_back(grow + ":");
    const auto capNonzero = newTemp("cap.nonzero");
    body.push_back("  " + capNonzero + " = icmp ne i64 " + cap + ", 0");
    const auto capDouble = newLabel("cap.double");
    const auto capInitial = newLabel("cap.initial");
    const auto capMerge = newLabel("cap.merge");
    body.push_back("  br i1 " + capNonzero + ", label %" + capDouble + ", label %" + capInitial);
    body.push_back(capInitial + ":");
    body.push_back("  br label %" + capMerge);
    body.push_back(capDouble + ":");
    const auto doubled = newTemp("cap.doubled");
    body.push_back("  " + doubled + " = add i64 " + cap + ", " + cap);
    const auto capWrapped = newTemp("cap.wrapped");
    body.push_back("  " + capWrapped + " = icmp ult i64 " + doubled + ", " + cap);
    const auto capBad = newLabel("cap.overflow");
    const auto capGood = newLabel("cap.ok");
    body.push_back("  br i1 " + capWrapped + ", label %" + capBad + ", label %" + capGood);
    body.push_back(capBad + ":");
    body.push_back("  call void @llvm.trap()");
    body.push_back("  unreachable");
    body.push_back(capGood + ":");
    body.push_back("  br label %" + capMerge);
    body.push_back(capMerge + ":");
    const auto newCap = newTemp("new.cap");
    body.push_back("  " + newCap + " = phi i64 [ 4, %" + capInitial + " ], [ " + doubled + ", %" + capGood + " ]");

    const auto elemSize = emitTypeSize(elemType);
    const auto bytesPair = newTemp("array.bytes.pair");
    body.push_back("  " + bytesPair + " = call { i64, i1 } @llvm.umul.with.overflow.i64(i64 " + newCap + ", i64 " + elemSize + ")");
    const auto bytes = newTemp("array.bytes");
    const auto bytesOv = newTemp("array.bytes.ov");
    body.push_back("  " + bytes + " = extractvalue { i64, i1 } " + bytesPair + ", 0");
    body.push_back("  " + bytesOv + " = extractvalue { i64, i1 } " + bytesPair + ", 1");
    const auto bytesBad = newLabel("array.bytes.overflow");
    const auto bytesGood = newLabel("array.bytes.ok");
    body.push_back("  br i1 " + bytesOv + ", label %" + bytesBad + ", label %" + bytesGood);
    body.push_back(bytesBad + ":");
    body.push_back("  call void @llvm.trap()");
    body.push_back("  unreachable");
    body.push_back(bytesGood + ":");

    const auto arenaPath = newLabel("array.alloc.arena");
    const auto heapPath = newLabel("array.alloc.heap");
    const auto allocMerge = newLabel("array.alloc.merge");
    const auto hasArena = newTemp("array.hasarena");
    body.push_back("  " + hasArena + " = icmp ne ptr " + arena + ", null");
    body.push_back("  br i1 " + hasArena + ", label %" + arenaPath + ", label %" + heapPath);
    body.push_back(arenaPath + ":");
    const auto arenaData = newTemp("array.arena.data");
    body.push_back("  " + arenaData + " = call ptr @__lanner_arena_alloc(ptr " + arena + ", i64 " + bytes + ")");
    body.push_back("  br label %" + allocMerge);
    body.push_back(heapPath + ":");
    const auto heapData = newTemp("array.heap.data");
    body.push_back("  " + heapData + " = call ptr @realloc(ptr " + data + ", i64 " + bytes + ")");
    const auto heapNull = newTemp("array.heap.null");
    body.push_back("  " + heapNull + " = icmp eq ptr " + heapData + ", null");
    const auto heapBad = newLabel("array.heap.oom");
    const auto heapGood = newLabel("array.heap.ok");
    body.push_back("  br i1 " + heapNull + ", label %" + heapBad + ", label %" + heapGood);
    body.push_back(heapBad + ":");
    body.push_back("  call void @llvm.trap()");
    body.push_back("  unreachable");
    body.push_back(heapGood + ":");
    body.push_back("  br label %" + allocMerge);
    body.push_back(allocMerge + ":");
    const auto newData = newTemp("array.newdata");
    body.push_back("  " + newData + " = phi ptr [ " + arenaData + ", %" + arenaPath + " ], [ " + heapData + ", %" + heapGood + " ]");

    const auto oldNonNull = newTemp("array.oldnonnull");
    body.push_back("  " + oldNonNull + " = icmp ne ptr " + data + ", null");
    const auto copy = newLabel("array.copy");
    const auto copySkip = newLabel("array.copy.skip");
    const auto copyDone = newLabel("array.copy.done");
    body.push_back("  br i1 " + oldNonNull + ", label %" + copy + ", label %" + copySkip);
    body.push_back(copySkip + ":");
    body.push_back("  br label %" + copyDone);
    body.push_back(copy + ":");
    const auto oldArena = newTemp("array.oldarena");
    body.push_back("  " + oldArena + " = icmp ne ptr " + arena + ", null");
    const auto copyArena = newLabel("array.copy.arena");
    const auto copyHeap = newLabel("array.copy.heap");
    const auto copyModeDone = newLabel("array.copy.mode.done");
    body.push_back("  br i1 " + oldArena + ", label %" + copyArena + ", label %" + copyHeap);
    body.push_back(copyArena + ":");
    const auto copyBytesPair = newTemp("copy.bytes.pair");
    body.push_back("  " + copyBytesPair + " = call { i64, i1 } @llvm.umul.with.overflow.i64(i64 " + len + ", i64 " + elemSize + ")");
    const auto copyBytes = newTemp("copy.bytes");
    const auto copyBytesOv = newTemp("copy.bytes.ov");
    body.push_back("  " + copyBytes + " = extractvalue { i64, i1 } " + copyBytesPair + ", 0");
    body.push_back("  " + copyBytesOv + " = extractvalue { i64, i1 } " + copyBytesPair + ", 1");
    const auto copyBad = newLabel("copy.overflow");
    const auto copyGood = newLabel("copy.ok");
    body.push_back("  br i1 " + copyBytesOv + ", label %" + copyBad + ", label %" + copyGood);
    body.push_back(copyBad + ":");
    body.push_back("  call void @llvm.trap()");
    body.push_back("  unreachable");
    body.push_back(copyGood + ":");
    body.push_back("  call void @llvm.memcpy.p0.p0.i64(ptr " + newData + ", ptr " + data + ", i64 " + copyBytes + ", i1 false)");
    body.push_back("  br label %" + copyModeDone);
    body.push_back(copyHeap + ":");
    // realloc already preserved heap data in-place/moved; nothing to copy.
    body.push_back("  br label %" + copyModeDone);
    body.push_back(copyModeDone + ":");
    body.push_back("  br label %" + copyDone);
    body.push_back(copyDone + ":");
    const auto grown0 = newTemp("array.g0");
    const auto grown1 = newTemp("array.g1");
    const auto grown2 = newTemp("array.g2");
    const auto grown3 = newTemp("array.g3");
    body.push_back("  " + grown0 + " = insertvalue %LannerDynArray " + array + ", ptr " + newData + ", 0");
    body.push_back("  " + grown1 + " = insertvalue %LannerDynArray " + grown0 + ", i64 " + len + ", 1");
    body.push_back("  " + grown2 + " = insertvalue %LannerDynArray " + grown1 + ", i64 " + newCap + ", 2");
    body.push_back("  " + grown3 + " = insertvalue %LannerDynArray " + grown2 + ", ptr " + arena + ", 3");
    body.push_back("  store %LannerDynArray " + grown3 + ", ptr " + targetAddr);
    body.push_back("  br label %" + write);

    body.push_back(write + ":");
    const auto currentArray = newTemp("array.current");
    const auto currentData = newTemp("array.current.data");
    const auto currentLen = newTemp("array.current.len");
    body.push_back("  " + currentArray + " = load %LannerDynArray, ptr " + targetAddr);
    body.push_back("  " + currentData + " = extractvalue %LannerDynArray " + currentArray + ", 0");
    body.push_back("  " + currentLen + " = extractvalue %LannerDynArray " + currentArray + ", 1");
    const auto addr = newTemp("array.push.addr");
    body.push_back("  " + addr + " = getelementptr inbounds " + elemType + ", ptr " + currentData + ", i64 " + currentLen);
    body.push_back("  store " + elemType + " " + valueIR + ", ptr " + addr);
    const auto newLen = newTemp("array.newlen");
    body.push_back("  " + newLen + " = add i64 " + currentLen + ", 1");
    const auto finalArray = newTemp("array.final");
    body.push_back("  " + finalArray + " = insertvalue %LannerDynArray " + currentArray + ", i64 " + newLen + ", 1");
    body.push_back("  store %LannerDynArray " + finalArray + ", ptr " + targetAddr);
    return "";
}

std::string LLVMCodeGenerator::emitDynamicArrayLiteral(const Expr* expr, const std::string& arenaAddress) {
    if (!expr->checkedType || !expr->checkedType->isArray || expr->checkedType->fixedArraySize) {
        unsupported("dynamic array literal requires []T type", expr->line);
    }
    const auto elemType = dynamicArrayElementType(expr->checkedType.get());
    const std::uint64_t count = static_cast<std::uint64_t>(expr->args.size());
    if (count == 0) {
        if (arenaAddress.empty()) return "zeroinitializer";
        const auto a0 = newTemp("empty.a0");
        const auto a1 = newTemp("empty.a1");
        const auto a2 = newTemp("empty.a2");
        body.push_back("  " + a0 + " = insertvalue %LannerDynArray zeroinitializer, ptr " + arenaAddress + ", 3");
        return a0;
    }

    const auto elemSize = emitTypeSize(elemType);
    const auto pair = newTemp("literal.bytes.pair");
    body.push_back("  " + pair + " = call { i64, i1 } @llvm.umul.with.overflow.i64(i64 " +
                   std::to_string(count) + ", i64 " + elemSize + ")");
    const auto bytes = newTemp("literal.bytes");
    const auto overflow = newTemp("literal.overflow");
    body.push_back("  " + bytes + " = extractvalue { i64, i1 } " + pair + ", 0");
    body.push_back("  " + overflow + " = extractvalue { i64, i1 } " + pair + ", 1");
    const auto good = newLabel("literal.bytes.ok");
    const auto bad = newLabel("literal.bytes.trap");
    body.push_back("  br i1 " + overflow + ", label %" + bad + ", label %" + good);
    body.push_back(bad + ":");
    body.push_back("  call void @llvm.trap()");
    body.push_back("  unreachable");
    body.push_back(good + ":");

    std::string data;
    std::string arenaPtr = arenaAddress.empty() ? "null" : arenaAddress;
    if (arenaAddress.empty()) {
        data = newTemp("literal.heap.data");
        body.push_back("  " + data + " = call ptr @malloc(i64 " + bytes + ")");
        const auto oom = newTemp("literal.oom");
        body.push_back("  " + oom + " = icmp eq ptr " + data + ", null");
        const auto oomBlock = newLabel("literal.oom.trap");
        const auto okBlock = newLabel("literal.alloc.ok");
        body.push_back("  br i1 " + oom + ", label %" + oomBlock + ", label %" + okBlock);
        body.push_back(oomBlock + ":");
        body.push_back("  call void @llvm.trap()");
        body.push_back("  unreachable");
        body.push_back(okBlock + ":");
    } else {
        data = newTemp("literal.arena.data");
        body.push_back("  " + data + " = call ptr @__lanner_arena_alloc(ptr " + arenaAddress + ", i64 " + bytes + ")");
    }

    for (std::size_t i = 0; i < expr->args.size(); ++i) {
        const auto value = emitExpr(expr->args[i].get());
        const auto address = newTemp("literal.elem.addr");
        body.push_back("  " + address + " = getelementptr inbounds " + elemType + ", ptr " + data + ", i64 " + std::to_string(i));
        body.push_back("  store " + elemType + " " + value + ", ptr " + address);
        markMovedArgument(expr->args[i].get(), nullptr);
    }

    const auto a0 = newTemp("literal.a0");
    const auto a1 = newTemp("literal.a1");
    const auto a2 = newTemp("literal.a2");
    const auto a3 = newTemp("literal.a3");
    body.push_back("  " + a0 + " = insertvalue %LannerDynArray zeroinitializer, ptr " + data + ", 0");
    body.push_back("  " + a1 + " = insertvalue %LannerDynArray " + a0 + ", i64 " + std::to_string(count) + ", 1");
    body.push_back("  " + a2 + " = insertvalue %LannerDynArray " + a1 + ", i64 " + std::to_string(count) + ", 2");
    body.push_back("  " + a3 + " = insertvalue %LannerDynArray " + a2 + ", ptr " + arenaPtr + ", 3");
    return a3;
}

const StructField* LLVMCodeGenerator::findStructField(const TypeNode* type, const std::string& field) const {
    if (!type) return nullptr;
    const auto it = structs.find(type->name);
    if (it == structs.end()) return nullptr;
    for (const auto& f : it->second->fields) {
        if (f.name == field) return &f;
    }
    return nullptr;
}

std::string LLVMCodeGenerator::emitLValueAddress(const Expr* expr) {
    if (!expr) unsupported("null assignment target", 0);

    if (expr->kind == ExprKind::Identifier) {
        auto* local = lookupLocal(expr->strValue);
        if (!local) {
            auto it = statics.find(expr->strValue);
            if (it != statics.end()) return "@" + expr->strValue;
            unsupported("unknown assignment target '" + expr->strValue + "'", expr->line);
        }
        if (local->type->isReference) {
            const auto object = newTemp("ref.lvalue");
            body.push_back("  " + object + " = load ptr, ptr " + local->addr);
            return object;
        }
        return local->addr;
    }

    if (expr->kind == ExprKind::UnaryOp && expr->op == "*") {
        return emitExpr(expr->value.get());
    }

    if (expr->kind == ExprKind::FieldAccess) {
        if (expr->target->kind == ExprKind::Identifier) {
            auto* local = lookupLocal(expr->target->strValue);
            if (local) {
                const auto* field = findStructField(local->type, expr->field);
                if (!field) unsupported("unknown struct field '" + expr->field + "'", expr->line);
                // llvmType() lowers any reference type to the opaque "ptr", which is
                // correct for the value itself but wrong as a GEP source element type:
                // the GEP must be typed by the pointee struct, not by "ptr" (which,
                // as a scalar, accepts only one index -- a second index there is
                // invalid IR). Strip isReference before asking llvmType for the
                // struct's layout type.
                std::string structType;
                if (local->type->isReference) {
                    auto pointee = cloneType(local->type);
                    pointee->isReference = false;
                    structType = llvmType(pointee.get());
                } else {
                    structType = llvmType(local->type);
                }
                const auto index = [&]() -> std::size_t {
                    const auto* st = structs.at(local->type->name);
                    for (std::size_t i = 0; i < st->fields.size(); ++i) if (st->fields[i].name == expr->field) return i;
                    return 0;
                }();
                const auto address = newTemp("field.addr");
                if (local->type->isReference) {
                    const auto object = newTemp("ref.obj");
                    body.push_back("  " + object + " = load ptr, ptr " + local->addr);
                    body.push_back("  " + address + " = getelementptr inbounds " + structType + ", ptr " + object + ", i32 0, i32 " + std::to_string(index));
                } else {
                    body.push_back("  " + address + " = getelementptr inbounds " + structType + ", ptr " + local->addr + ", i32 0, i32 " + std::to_string(index));
                }
                return address;
            }
        }

        const auto* targetType = expr->target->checkedType.get();
        if (!targetType) unsupported("field assignment target has no type annotation", expr->line);
        const auto* field = findStructField(targetType, expr->field);
        if (!field) unsupported("unknown struct field '" + expr->field + "'", expr->line);
        const auto index = [&]() -> std::size_t {
            const auto* st = structs.at(targetType->name);
            for (std::size_t i = 0; i < st->fields.size(); ++i) if (st->fields[i].name == expr->field) return i;
            return 0;
        }();
        const auto targetAddr = emitLValueAddress(expr->target.get());
        const auto address = newTemp("field.addr");
        body.push_back("  " + address + " = getelementptr inbounds " + llvmType(targetType) + ", ptr " + targetAddr + ", i32 0, i32 " + std::to_string(index));
        return address;
    }

    if (expr->kind == ExprKind::Index) {
        if (expr->args.size() != 1) unsupported("index assignment requires one index", expr->line);
        const auto indexValue = emitExpr(expr->args[0].get());
        const auto indexI64 = normalizeIndexToI64(expr->args[0].get(), indexValue);
        const auto* sourceType = expr->target->checkedType.get();
        if (!sourceType) unsupported("indexed target has no type annotation", expr->line);

        auto normalizedSource = cloneType(sourceType);
        if (normalizedSource->isReference) normalizedSource->isReference = false;

        if (normalizedSource->isArray && normalizedSource->fixedArraySize) {
            emitBoundsCheck(expr->args[0].get(), indexI64, *normalizedSource->fixedArraySize);
            const auto arrayType = llvmType(normalizedSource.get());
            std::string base;
            if (sourceType->isReference) {
                base = emitExpr(expr->target.get());
            } else {
                base = emitLValueAddress(expr->target.get());
            }
            const auto address = newTemp("elem.addr");
            body.push_back("  " + address + " = getelementptr inbounds " + arrayType + ", ptr " + base + ", i64 0, i64 " + indexI64);
            return address;
        }

        if (normalizedSource->isRawPointer) {
            if (normalizedSource->name == "void") unsupported("cannot index a void raw pointer", expr->line);
            const auto base = emitExpr(expr->target.get());
            const auto pointee = [&]() {
                auto p = cloneType(normalizedSource.get());
                p->isRawPointer = false;
                p->isMutable = false;
                p->origin = {};
                p->nestedOrigins.clear();
                return p;
            }();
            const auto address = newTemp("raw.elem.addr");
            // Raw-pointer indexing is explicitly unsafe and intentionally performs
            // no bounds/provenance checks. This is the low-level escape hatch.
            body.push_back("  " + address + " = getelementptr " + llvmType(pointee.get()) + ", ptr " + base + ", i64 " + indexI64);
            return address;
        }

        if (normalizedSource->isArray && !normalizedSource->fixedArraySize) {
            std::string array;
            if (sourceType->isReference) {
                const auto arrayPtr = emitExpr(expr->target.get());
                array = newTemp("array.ref.load");
                body.push_back("  " + array + " = load %LannerDynArray, ptr " + arrayPtr);
            } else {
                array = emitExpr(expr->target.get());
            }
            const auto base = newTemp("array.base");
            const auto length = newTemp("array.length");
            body.push_back("  " + base + " = extractvalue %LannerDynArray " + array + ", 0");
            body.push_back("  " + length + " = extractvalue %LannerDynArray " + array + ", 1");
            emitDynamicBoundsCheck(indexI64, length);
            const auto elemType = dynamicArrayElementType(normalizedSource.get());
            const auto address = newTemp("array.elem.addr");
            body.push_back("  " + address + " = getelementptr inbounds " + elemType + ", ptr " + base + ", i64 " + indexI64);
            return address;
        }

        if ((normalizedSource->name == "View" || normalizedSource->name == "EditView") && normalizedSource->generics.size() == 1) {
            const auto view = sourceType->isReference ? emitExpr(expr->target.get()) : emitExpr(expr->target.get());
            const auto actualView = sourceType->isReference ? [&]() {
                const auto loaded = newTemp("view.ref.load");
                body.push_back("  " + loaded + " = load { ptr, i64 }, ptr " + view);
                return loaded;
            }() : view;
            const auto base = newTemp("view.ptr");
            const auto length = newTemp("view.len");
            body.push_back("  " + base + " = extractvalue { ptr, i64 } " + actualView + ", 0");
            body.push_back("  " + length + " = extractvalue { ptr, i64 } " + actualView + ", 1");
            emitDynamicBoundsCheck(indexI64, length);
            const auto elemType = llvmType(normalizedSource->generics[0].get());
            const auto address = newTemp("view.elem.addr");
            body.push_back("  " + address + " = getelementptr inbounds " + elemType + ", ptr " + base + ", i64 " + indexI64);
            return address;
        }

        unsupported("only fixed arrays and View values support indexed assignment", expr->line);
    }

    unsupported("assignment target is not lowered", expr->line);
}

std::string LLVMCodeGenerator::llvmStringLiteral(const std::string& value) {
    const std::string name = ".lanner.str." + std::to_string(stringLiterals.size());
    std::string encoded;
    encoded.reserve(value.size() * 3 + 3);
    for (unsigned char c : value) {
        if (c >= 0x20 && c <= 0x7E && c != '"' && c != '\\') encoded.push_back(static_cast<char>(c));
        else encoded += hexByte(c);
    }
    encoded += "\\00";
    stringLiterals.push_back("@" + name + " = private unnamed_addr constant [" +
                             std::to_string(value.size() + 1) + " x i8] c\"" + encoded + "\"");
    const auto result = newTemp("str");
    body.push_back("  " + result + " = getelementptr inbounds [" + std::to_string(value.size() + 1) +
                   " x i8], ptr @" + name + ", i64 0, i64 0");
    return result;
}

std::string LLVMCodeGenerator::emitPrintCall(const Expr* expr) {
    if (!expr || expr->callee->kind != ExprKind::Identifier || expr->args.size() != 1) {
        unsupported("invalid print() call", expr ? expr->line : 0);
    }
    const Expr* arg = expr->args[0].get();
    if (!arg->checkedType) unsupported("print() argument has no checked type", expr->line);
    const TypeNode* type = arg->checkedType.get();


    if (isWebTarget()) {
        const auto value = emitExpr(arg);
        if (type->name == "string" && !type->isArray && !type->isReference && !type->isOptional) {
            body.push_back("  call void @__lanner_web_log(ptr " + value + ")");
            return {};
        }
        if (type->name == "bool" && !type->isArray && !type->isReference && !type->isOptional) {
            body.push_back("  call void @__lanner_web_log_bool(i1 " + value + ")");
            return {};
        }
        if ((type->name == "f32" || type->name == "f64") && !type->isArray && !type->isReference && !type->isOptional) {
            auto v = value;
            if (exprLLVMType(arg) == "float") {
                const auto widened = newTemp("web.print.f64");
                body.push_back("  " + widened + " = fpext float " + v + " to double");
                v = widened;
            }
            body.push_back("  call void @__lanner_web_log_f64(double " + v + ")");
            return {};
        }
        if (isIntegerLLVM(llvmType(type)) && !type->isArray && !type->isReference && !type->isOptional) {
            auto v = value;
            const auto vt = exprLLVMType(arg);
            if (vt != "i64") {
                const auto widened = newTemp("web.print.i64");
                body.push_back("  " + widened + " = " + std::string(isUnsignedType(type) ? "zext " : "sext ") + vt + " " + v + " to i64");
                v = widened;
            }
            body.push_back("  call void @__lanner_web_log_i64(i64 " + v + ")");
            return {};
        }
        unsupported("print() argument type is not printable in WebAssembly", expr->line);
    }

    if (type->name == "string" && !type->isArray && !type->isReference && !type->isOptional) {
        const auto value = emitExpr(arg);
        body.push_back("  call i32 @puts(ptr " + value + ")");
        return {};
    }
    if (type->name == "bool" && !type->isArray && !type->isReference && !type->isOptional) {
        const auto value = emitExpr(arg);
        body.push_back("  call void @__lanner_print_bool(i1 " + value + ")");
        return {};
    }
    if ((type->name == "f32" || type->name == "f64") && !type->isArray && !type->isReference && !type->isOptional) {
        auto value = emitExpr(arg);
        if (exprLLVMType(arg) == "float") {
            const auto widened = newTemp("print.f64");
            body.push_back("  " + widened + " = fpext float " + value + " to double");
            value = widened;
        }
        body.push_back("  call void @__lanner_print_f64(double " + value + ")");
        return {};
    }
    if (isIntegerLLVM(llvmType(type)) && !type->isArray && !type->isReference && !type->isOptional) {
        auto value = emitExpr(arg);
        const auto llvmValueType = exprLLVMType(arg);
        if (llvmValueType != "i64") {
            const auto widened = newTemp("print.i64");
            const char* op = isUnsignedType(type) ? "zext" : "sext";
            body.push_back("  " + widened + " = " + op + " " + llvmValueType + " " + value + " to i64");
            value = widened;
        }
        if (isUnsignedType(type)) body.push_back("  call void @__lanner_print_u64(i64 " + value + ")");
        else body.push_back("  call void @__lanner_print_i64(i64 " + value + ")");
        return {};
    }
    unsupported("print() argument type is not printable", expr->line);
}

std::string LLVMCodeGenerator::emitBuiltinCall(const Expr* expr) {
    if (!expr) return {};
    if (expr->callee && expr->callee->kind == ExprKind::FieldAccess && expr->callee->target->kind == ExprKind::Identifier) {
        const auto ns = expr->callee->target->strValue;
        const auto member = expr->callee->field;
        if (ns == "Atomic" && member == "new") {
            return emitExpr(expr->args[0].get());
        }
        if (ns == "Game") {
            auto arg = [&](std::size_t i) { return emitExpr(expr->args[i].get()); };
            auto i32At = [&](std::size_t i) {
                auto v = arg(i); auto t = exprLLVMType(expr->args[i].get());
                if (t != "i32") {
                    auto w = newTemp("game.i32");
                    const bool signedValue = expr->args[i]->checkedType && isSignedType(expr->args[i]->checkedType.get());
                    body.push_back("  " + w + " = " + (signedValue ? "sext " : "zext ") + t + " " + v + " to i32"); v = w;
                }
                return v;
            };
            auto u32At = [&](std::size_t i) {
                auto v = arg(i); auto t = exprLLVMType(expr->args[i].get());
                if (t != "i32") { auto w = newTemp("game.u32"); body.push_back("  " + w + " = zext " + t + " " + v + " to i32"); v = w; }
                return v;
            };
            auto u64At = [&](std::size_t i) {
                auto v = arg(i); auto t = exprLLVMType(expr->args[i].get());
                if (t != "i64") { auto w = newTemp("game.u64"); body.push_back("  " + w + " = zext " + t + " " + v + " to i64"); v = w; }
                return v;
            };
            if (member == "createWindow") { auto r=newTemp("game.window"); body.push_back("  "+r+" = call ptr @__lanner_game_window_create(ptr "+arg(0)+", i32 "+i32At(1)+", i32 "+i32At(2)+", i32 "+u32At(3)+")"); return r; }
            if (member == "destroyWindow") { body.push_back("  call void @__lanner_game_window_destroy(ptr "+arg(0)+")"); return {}; }
            if (member == "poll") { auto r=newTemp("game.poll"); body.push_back("  "+r+" = call i32 @__lanner_game_poll(ptr "+arg(0)+")"); return r; }
            if (member == "shouldClose") { auto r=newTemp("game.close"); body.push_back("  "+r+" = call i32 @__lanner_game_should_close(ptr "+arg(0)+")"); auto b=newTemp("game.close.bool"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b; }
            if (member == "requestClose") { body.push_back("  call void @__lanner_game_request_close(ptr "+arg(0)+")"); return {}; }
            if (member == "setTitle") { body.push_back("  call void @__lanner_game_set_title(ptr "+arg(0)+", ptr "+arg(1)+")"); return {}; }
            if (member == "width" || member == "height") { auto r=newTemp("game.size"); body.push_back("  "+r+" = call i32 @__lanner_game_window_"+member+"(ptr "+arg(0)+")"); return r; }
            if (member == "setVSync") { auto r=newTemp("game.vsync"); body.push_back("  "+r+" = call i32 @__lanner_game_set_vsync(ptr "+arg(0)+", i1 "+arg(1)+")"); auto b=newTemp("game.vsync.bool"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b; }
            if (member == "makeGLContext") { auto r=newTemp("game.gl"); body.push_back("  "+r+" = call i32 @__lanner_game_make_gl_context(ptr "+arg(0)+")"); auto b=newTemp("game.gl.bool"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b; }
            if (member == "present") { body.push_back("  call void @__lanner_game_present(ptr "+arg(0)+")"); return {}; }
            if (member == "windowFlags") { auto r=newTemp("game.flags"); body.push_back("  "+r+" = call i32 @__lanner_game_window_flags(i1 "+arg(0)+", i1 "+arg(1)+", i1 "+arg(2)+", i1 "+arg(3)+")"); return r; }
            if (member == "rendererFlags") { auto r=newTemp("game.renderer.flags"); body.push_back("  "+r+" = call i32 @__lanner_game_renderer_flags(i1 "+arg(0)+", i1 "+arg(1)+")"); return r; }
            if (member == "createRenderer") { auto r=newTemp("game.renderer"); body.push_back("  "+r+" = call ptr @__lanner_game_renderer_create(ptr "+arg(0)+", i32 "+u32At(1)+")"); return r; }
            if (member == "destroyRenderer") { body.push_back("  call void @__lanner_game_renderer_destroy(ptr "+arg(0)+")"); return {}; }
            if (member == "setDrawColor") { auto r=newTemp("game.color"); body.push_back("  "+r+" = call i32 @__lanner_game_renderer_set_color(ptr "+arg(0)+", i8 "+arg(1)+", i8 "+arg(2)+", i8 "+arg(3)+", i8 "+arg(4)+")"); auto b=newTemp("game.color.bool"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b; }
            if (member == "clear") { auto r=newTemp("game.clear"); body.push_back("  "+r+" = call i32 @__lanner_game_renderer_clear(ptr "+arg(0)+")"); auto b=newTemp("game.clear.bool"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b; }
            if (member == "drawLine") { auto r=newTemp("game.line"); body.push_back("  "+r+" = call i32 @__lanner_game_renderer_line(ptr "+arg(0)+", i32 "+i32At(1)+", i32 "+i32At(2)+", i32 "+i32At(3)+", i32 "+i32At(4)+")"); auto b=newTemp("game.line.bool"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b; }
            if (member == "fillRect") { auto r=newTemp("game.rect"); body.push_back("  "+r+" = call i32 @__lanner_game_renderer_fill_rect(ptr "+arg(0)+", i32 "+i32At(1)+", i32 "+i32At(2)+", i32 "+i32At(3)+", i32 "+i32At(4)+")"); auto b=newTemp("game.rect.bool"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b; }
            if (member == "presentRenderer") { body.push_back("  call void @__lanner_game_renderer_present(ptr "+arg(0)+")"); return {}; }
            if (member == "createTexture") { auto r=newTemp("game.texture"); body.push_back("  "+r+" = call ptr @__lanner_game_texture_create(ptr "+arg(0)+", i32 "+u32At(1)+", i32 "+u32At(2)+", i32 "+i32At(3)+", i32 "+i32At(4)+")"); return r; }
            if (member == "updateTexture") { auto p = arg(1); auto pitch = i32At(2); auto r=newTemp("game.texture.update"); body.push_back("  "+r+" = call i32 @__lanner_game_texture_update(ptr "+arg(0)+", ptr "+p+", i32 "+pitch+")"); auto b=newTemp("game.texture.update.bool"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b; }
            if (member == "copyTexture") { auto r=newTemp("game.texture.copy"); body.push_back("  "+r+" = call i32 @__lanner_game_texture_copy(ptr "+arg(0)+", ptr "+arg(1)+", i32 "+i32At(2)+", i32 "+i32At(3)+", i32 "+i32At(4)+")"); auto b=newTemp("game.texture.copy.bool"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b; }
            if (member == "destroyTexture") { body.push_back("  call void @__lanner_game_texture_destroy(ptr "+arg(0)+")"); return {}; }
            if (member == "eventType" || member == "eventCode" || member == "eventX" || member == "eventY") { const char* f=member=="eventType"?"type":member=="eventCode"?"code":member=="eventX"?"x":"y"; auto r=newTemp("game.event"); body.push_back("  "+r+" = call i32 @__lanner_game_event_"+std::string(f)+"()"); return r; }
            if (member == "eventText") { auto r=newTemp("game.event.text"); body.push_back("  "+r+" = call ptr @__lanner_game_event_text()"); return r; }
            if (member == "keyDown") { auto r=newTemp("game.key"); body.push_back("  "+r+" = call i32 @__lanner_game_key_down(i32 "+i32At(0)+")"); auto b=newTemp("game.key.bool"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b; }
            if (member == "mouseButtonDown") { auto r=newTemp("game.mouse.button"); body.push_back("  "+r+" = call i32 @__lanner_game_mouse_button_down(i32 "+i32At(0)+")"); auto b=newTemp("game.mouse.button.bool"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b; }
            if (member == "mouseX" || member == "mouseY") { const char* axis = member == "mouseX" ? "x" : "y"; auto r=newTemp("game.mouse"); body.push_back("  "+r+" = call i32 @__lanner_game_mouse_"+std::string(axis)+"()"); return r; }
            if (member == "controllerConnected") { auto r=newTemp("game.pad"); body.push_back("  "+r+" = call i32 @__lanner_game_controller_connected(i32 "+i32At(0)+")"); auto b=newTemp("game.pad.bool"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b; }
            if (member == "controllerAxis") { auto r=newTemp("game.axis"); body.push_back("  "+r+" = call float @__lanner_game_controller_axis(i32 "+i32At(0)+", i32 "+i32At(1)+")"); return r; }
            if (member == "controllerButtonDown") { auto r=newTemp("game.pad.button"); body.push_back("  "+r+" = call i32 @__lanner_game_controller_button(i32 "+i32At(0)+", i32 "+i32At(1)+")"); auto b=newTemp("game.pad.button.bool"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b; }
            if (member == "audioOpen") { auto r=newTemp("game.audio"); body.push_back("  "+r+" = call ptr @__lanner_game_audio_open(i32 "+i32At(0)+", i32 "+i32At(1)+", i32 "+i32At(2)+")"); return r; }
            if (member == "audioWrite") { auto r64=newTemp("game.audio.write64"); body.push_back("  "+r64+" = call i64 @__lanner_game_audio_write(ptr "+arg(0)+", ptr "+arg(1)+", i64 "+u64At(2)+")"); if(pointerBits==64)return r64; auto r=newTemp("game.audio.write"); body.push_back("  "+r+" = trunc i64 "+r64+" to i32"); return r; }
            if (member == "audioQueued") { auto r64=newTemp("game.audio.queued64"); body.push_back("  "+r64+" = call i64 @__lanner_game_audio_queued(ptr "+arg(0)+")"); if(pointerBits==64)return r64; auto r=newTemp("game.audio.queued"); body.push_back("  "+r+" = trunc i64 "+r64+" to i32"); return r; }
            if (member == "audioPause") { body.push_back("  call void @__lanner_game_audio_pause(ptr "+arg(0)+", i1 "+arg(1)+")"); return {}; }
            if (member == "audioClose") { body.push_back("  call void @__lanner_game_audio_close(ptr "+arg(0)+")"); return {}; }
            if (member == "timeNanos") { auto r=newTemp("game.time"); body.push_back("  "+r+" = call i64 @__lanner_game_time_nanos()"); return r; }
            if (member == "deltaSeconds") { auto r=newTemp("game.delta"); body.push_back("  "+r+" = call double @__lanner_game_delta_seconds()"); return r; }
            if (member == "sleepNanos") { body.push_back("  call void @__lanner_game_sleep_nanos(i64 "+arg(0)+")"); return {}; }
        }
        if (ns == "Graphics") {
            auto arg = [&](std::size_t i) { return emitExpr(expr->args[i].get()); };
            auto i32At = [&](std::size_t i) { auto v=arg(i); auto t=exprLLVMType(expr->args[i].get()); if(t!="i32"){auto w=newTemp("gfx.i32");const bool sgn=expr->args[i]->checkedType&&isSignedType(expr->args[i]->checkedType.get());body.push_back("  "+w+" = "+(sgn?"sext ":"zext ")+t+" "+v+" to i32");v=w;}return v; };
            auto u32At = [&](std::size_t i) { auto v=arg(i); auto t=exprLLVMType(expr->args[i].get()); if(t!="i32"){auto w=newTemp("gfx.u32");body.push_back("  "+w+" = zext "+t+" "+v+" to i32");v=w;}return v; };
            auto i64At = [&](std::size_t i) { auto v=arg(i); auto t=exprLLVMType(expr->args[i].get()); if(t!="i64"){auto w=newTemp("gfx.i64");body.push_back("  "+w+" = zext "+t+" "+v+" to i64");v=w;}return v; };
            if(member=="available"){auto r=newTemp("gfx.available");body.push_back("  "+r+" = call i32 @__lanner_gfx_available(ptr "+arg(0)+")");auto b=newTemp("gfx.available.bool");body.push_back("  "+b+" = trunc i32 "+r+" to i1");return b;}
            if(member=="backend"){auto r=newTemp("gfx.backend");body.push_back("  "+r+" = call ptr @__lanner_gfx_backend()");return r;}
            if(member=="loadProc"){auto r=newTemp("gfx.proc");body.push_back("  "+r+" = call ptr @__lanner_gfx_load_proc(ptr "+arg(0)+", ptr "+arg(1)+")");return r;}
            if(member=="glClearColor"){body.push_back("  call void @__lanner_gl_clear_color(float "+arg(0)+", float "+arg(1)+", float "+arg(2)+", float "+arg(3)+")");return {};}
            if(member=="glClear"){body.push_back("  call void @__lanner_gl_clear(i32 "+u32At(0)+")");return {};}
            if(member=="glViewport"){body.push_back("  call void @__lanner_gl_viewport(i32 "+i32At(0)+", i32 "+i32At(1)+", i32 "+i32At(2)+", i32 "+i32At(3)+")");return {};}
            if(member=="glEnable"||member=="glDisable"){body.push_back("  call void @__lanner_gl_"+std::string(member=="glEnable"?"enable":"disable")+"(i32 "+u32At(0)+")");return {};}
            if(member=="glGenBuffers"||member=="glGenVertexArrays"){body.push_back("  call void @__lanner_gl_"+std::string(member=="glGenBuffers"?"gen_buffers":"gen_vertex_arrays")+"(i32 "+i32At(0)+", ptr "+arg(1)+")");return {};}
            if(member=="glBindBuffer"){body.push_back("  call void @__lanner_gl_bind_buffer(i32 "+u32At(0)+", i32 "+u32At(1)+")");return {};}
            if(member=="glBufferData"){body.push_back("  call void @__lanner_gl_buffer_data(i32 "+u32At(0)+", i64 "+i64At(1)+", ptr "+arg(2)+", i32 "+u32At(3)+")");return {};}
            if(member=="glCreateShader"){auto r=newTemp("gfx.shader");body.push_back("  "+r+" = call i32 @__lanner_gl_create_shader(i32 "+u32At(0)+")");return r;}
            if(member=="glShaderSource"){body.push_back("  call void @__lanner_gl_shader_source(i32 "+u32At(0)+", ptr "+arg(1)+")");return {};}
            if(member=="glCompileShader"){body.push_back("  call void @__lanner_gl_compile_shader(i32 "+u32At(0)+")");return {};}
            if(member=="glShaderStatus"){auto r=newTemp("gfx.shader.ok");body.push_back("  "+r+" = call i32 @__lanner_gl_shader_status(i32 "+u32At(0)+")");auto b=newTemp("gfx.shader.ok.bool");body.push_back("  "+b+" = trunc i32 "+r+" to i1");return b;}
            if(member=="glShaderLog"){auto r=newTemp("gfx.shader.log");body.push_back("  "+r+" = call ptr @__lanner_gl_shader_log(i32 "+u32At(0)+")");return r;}
            if(member=="glCreateProgram"){auto r=newTemp("gfx.program");body.push_back("  "+r+" = call i32 @__lanner_gl_create_program()");return r;}
            if(member=="glAttachShader"){body.push_back("  call void @__lanner_gl_attach_shader(i32 "+u32At(0)+", i32 "+u32At(1)+")");return {};}
            if(member=="glLinkProgram"){body.push_back("  call void @__lanner_gl_link_program(i32 "+u32At(0)+")");return {};}
            if(member=="glProgramStatus"){auto r=newTemp("gfx.program.ok");body.push_back("  "+r+" = call i32 @__lanner_gl_program_status(i32 "+u32At(0)+")");auto b=newTemp("gfx.program.ok.bool");body.push_back("  "+b+" = trunc i32 "+r+" to i1");return b;}
            if(member=="glProgramLog"){auto r=newTemp("gfx.program.log");body.push_back("  "+r+" = call ptr @__lanner_gl_program_log(i32 "+u32At(0)+")");return r;}
            if(member=="glUseProgram"){body.push_back("  call void @__lanner_gl_use_program(i32 "+u32At(0)+")");return {};}
            if(member=="glDrawArrays"){body.push_back("  call void @__lanner_gl_draw_arrays(i32 "+u32At(0)+", i32 "+i32At(1)+", i32 "+i32At(2)+")");return {};}
            if(member=="glBindVertexArray"){body.push_back("  call void @__lanner_gl_bind_vertex_array(i32 "+u32At(0)+")");return {};}
            if(member=="glEnableVertexAttribArray"){body.push_back("  call void @__lanner_gl_enable_vertex_attrib(i32 "+u32At(0)+")");return {};}
            if(member=="glVertexAttribPointer"){body.push_back("  call void @__lanner_gl_vertex_attrib_pointer(i32 "+i32At(0)+", i32 "+i32At(1)+", i32 "+u32At(2)+", i1 "+arg(3)+", i32 "+i32At(4)+", i64 "+i64At(5)+")");return {};}
            if(member=="glGetError"){auto r=newTemp("gfx.error");body.push_back("  "+r+" = call i32 @__lanner_gl_get_error()");return r;}
            if(member=="glDeleteShader"||member=="glDeleteProgram"){body.push_back("  call void @__lanner_gl_"+std::string(member=="glDeleteShader"?"delete_shader":"delete_program")+"(i32 "+u32At(0)+")");return {};}
            if(member=="glDeleteBuffers"||member=="glDeleteVertexArrays"){body.push_back("  call void @__lanner_gl_"+std::string(member=="glDeleteBuffers"?"delete_buffers":"delete_vertex_arrays")+"(i32 "+i32At(0)+", ptr "+arg(1)+")");return {};}
        }
        if (expr->callee->target && expr->callee->target->checkedType && expr->callee->target->checkedType->name == "Tensor") {
            auto base = [&]() { return emitExpr(expr->callee->target.get()); };
            auto arg = [&](std::size_t i) { return emitExpr(expr->args[i].get()); };
            auto asU64 = [&](std::size_t i) {
                auto v = emitExpr(expr->args[i].get()); auto t = exprLLVMType(expr->args[i].get());
                if (t != "i64") { auto w=newTemp("tensor.u64"); body.push_back("  "+w+" = zext "+t+" "+v+" to i64"); v=w; }
                return v;
            };
            auto asF64 = [&](std::size_t i) {
                auto v = emitExpr(expr->args[i].get()); auto t = exprLLVMType(expr->args[i].get());
                if (t == "float") { auto w=newTemp("tensor.f64"); body.push_back("  "+w+" = fpext float "+v+" to double"); v=w; }
                return v;
            };
            if (member == "clone" || member == "contiguous") { auto r=newTemp("tensor.clone"); body.push_back("  "+r+" = call ptr @"+std::string(member=="clone"?"__lanner_tensor_clone":"__lanner_tensor_contiguous")+"(ptr "+base()+")"); return r; }
            if (member == "free") { body.push_back("  call void @__lanner_tensor_free(ptr "+base()+")"); return {}; }
            if (member == "rank" || member == "len") { auto r=newTemp("tensor.query"); body.push_back("  "+r+" = call i64 @"+std::string(member=="rank"?"__lanner_tensor_rank":"__lanner_tensor_len")+"(ptr "+base()+")"); return r; }
            if (member == "dim" || member == "stride") { auto r=newTemp("tensor.query"); body.push_back("  "+r+" = call i64 @"+std::string(member=="dim"?"__lanner_tensor_dim":"__lanner_tensor_stride")+"(ptr "+base()+", i64 "+asU64(0)+")"); return r; }
            if (member == "dtype") { auto r=newTemp("tensor.dtype"); body.push_back("  "+r+" = call i32 @__lanner_tensor_dtype(ptr "+base()+")"); return r; }
            if (member == "isContiguous") { auto r=newTemp("tensor.contig"); body.push_back("  "+r+" = call i32 @__lanner_tensor_is_contiguous(ptr "+base()+")"); auto b=newTemp("tensor.contig.bool"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b; }
            if (member == "dataF32" || member == "dataF64") { auto r=newTemp("tensor.data"); body.push_back("  "+r+" = call ptr @"+std::string(member=="dataF32"?"__lanner_tensor_data_f32":"__lanner_tensor_data_f64")+"(ptr "+base()+")"); return r; }
            if (member == "get1" || member == "get2" || member == "get3") { const int n=member.back()-'0'; std::string a="ptr "+base(); for(int i=0;i<n;++i)a+=", i64 "+asU64(i); auto r=newTemp("tensor.get"); body.push_back("  "+r+" = call double @__lanner_tensor_get"+std::to_string(n)+"("+a+")"); return r; }
            if (member == "set1" || member == "set2" || member == "set3") { const int n=member.back()-'0'; std::string a="ptr "+base(); for(int i=0;i<n;++i)a+=", i64 "+asU64(i); a+=", double "+asF64(n); body.push_back("  call void @__lanner_tensor_set"+std::to_string(n)+"("+a+")"); return {}; }
            if (member == "add" || member == "sub" || member == "mul" || member == "div" || member == "matmul") { const char* f=member=="add"?"__lanner_tensor_add":member=="sub"?"__lanner_tensor_sub":member=="mul"?"__lanner_tensor_mul":member=="div"?"__lanner_tensor_div":"__lanner_tensor_matmul"; auto r=newTemp("tensor.op"); body.push_back("  "+r+" = call ptr @"+f+"(ptr "+base()+", ptr "+arg(0)+")"); return r; }
            if (member == "scale") { auto r=newTemp("tensor.scale"); body.push_back("  "+r+" = call ptr @__lanner_tensor_scale(ptr "+base()+", double "+asF64(0)+")"); return r; }
            if (member == "relu" || member == "sigmoid" || member == "tanh") { const char* f=member=="relu"?"__lanner_tensor_relu":member=="sigmoid"?"__lanner_tensor_sigmoid":"__lanner_tensor_tanh"; auto r=newTemp("tensor.act"); body.push_back("  "+r+" = call ptr @"+f+"(ptr "+base()+")"); return r; }
            if (member == "softmax") { auto r=newTemp("tensor.softmax"); body.push_back("  "+r+" = call ptr @__lanner_tensor_softmax(ptr "+base()+", i64 "+asU64(0)+")"); return r; }
            if (member == "sum" || member == "mean" || member == "l2Norm") { const char* f=member=="sum"?"__lanner_tensor_sum":member=="mean"?"__lanner_tensor_mean":"__lanner_tensor_l2norm"; auto r=newTemp("tensor.reduce"); body.push_back("  "+r+" = call double @"+f+"(ptr "+base()+")"); return r; }
            if (member == "dot") { auto r=newTemp("tensor.dot"); body.push_back("  "+r+" = call double @__lanner_tensor_dot(ptr "+base()+", ptr "+arg(0)+")"); return r; }
            if (member == "argmax") { auto r=newTemp("tensor.argmax"); body.push_back("  "+r+" = call i64 @__lanner_tensor_argmax(ptr "+base()+", i64 "+asU64(0)+")"); return r; }
            if (member == "reshape2" || member == "reshape3" || member == "reshape4") { const int n=member.back()-'0'; std::string a="ptr "+base(); for(int i=0;i<n;++i)a+=", i64 "+asU64(i); auto r=newTemp("tensor.reshape"); body.push_back("  "+r+" = call ptr @__lanner_tensor_reshape"+std::to_string(n)+"("+a+")"); return r; }
            if (member == "transpose2") { auto r=newTemp("tensor.transpose"); body.push_back("  "+r+" = call ptr @__lanner_tensor_transpose2(ptr "+base()+")"); return r; }
            if (member == "slice") { auto r=newTemp("tensor.slice"); body.push_back("  "+r+" = call ptr @__lanner_tensor_slice(ptr "+base()+", i64 "+asU64(0)+", i64 "+asU64(1)+", i64 "+asU64(2)+", i64 "+asU64(3)+")"); return r; }
            if (member == "fill") { body.push_back("  call void @__lanner_tensor_fill(ptr "+base()+", double "+asF64(0)+")"); return {}; }
            if (member == "conv2d") { auto r=newTemp("tensor.conv2d"); body.push_back("  "+r+" = call ptr @__lanner_tensor_conv2d(ptr "+base()+", ptr "+arg(0)+", i64 "+asU64(1)+", i64 "+asU64(2)+")"); return r; }
        }
        if (ns == "Tensor") {
            auto asU64 = [&](std::size_t i) {
                auto v = emitExpr(expr->args[i].get());
                auto t = exprLLVMType(expr->args[i].get());
                if (t != "i64") {
                    auto w = newTemp("tensor.u64");
                    body.push_back("  " + w + " = zext " + t + " " + v + " to i64");
                    v = w;
                }
                return v;
            };
            auto asF64 = [&](std::size_t i) {
                auto v = emitExpr(expr->args[i].get());
                auto t = exprLLVMType(expr->args[i].get());
                if (t == "float") {
                    auto w = newTemp("tensor.f64");
                    body.push_back("  " + w + " = fpext float " + v + " to double");
                    v = w;
                }
                return v;
            };
            auto arg = [&](std::size_t i) { return emitExpr(expr->args[i].get()); };
            if (member == "zerosF32" || member == "onesF32" || member == "zerosF64" || member == "onesF64") {
                auto n = asU64(0);
                const char* f = member == "zerosF32" ? "__lanner_tensor_zeros_f32" :
                                member == "onesF32" ? "__lanner_tensor_ones_f32" :
                                member == "zerosF64" ? "__lanner_tensor_zeros_f64" : "__lanner_tensor_ones_f64";
                auto r = newTemp("tensor.new");
                body.push_back("  " + r + " = call ptr @" + std::string(f) + "(i64 " + n + ")");
                return r;
            }
            if (member == "zeros1" || member == "zeros2" || member == "zeros3" || member == "zeros4" ||
                member == "ones1" || member == "ones2" || member == "ones3" || member == "ones4") {
                const int rank = member.back() - '0';
                std::string args;
                for (int i = 0; i < rank; ++i) {
                    if (i) args += ", ";
                    args += "i64 " + asU64(static_cast<std::size_t>(i));
                }
                const bool ones = member[0] == 'o';
                const char* f = rank == 1 ? (ones ? "__lanner_tensor_ones1" : "__lanner_tensor_zeros1") :
                                rank == 2 ? (ones ? "__lanner_tensor_ones2" : "__lanner_tensor_zeros2") :
                                rank == 3 ? (ones ? "__lanner_tensor_ones3" : "__lanner_tensor_zeros3") :
                                            (ones ? "__lanner_tensor_ones4" : "__lanner_tensor_zeros4");
                auto r = newTemp("tensor.new");
                body.push_back("  " + r + " = call ptr @" + std::string(f) + "(" + args + ")");
                return r;
            }
            if (member == "from1F32" || member == "from1F64") {
                auto p0 = arg(0), n = asU64(1);
                const char* f = member == "from1F32" ? "__lanner_tensor_from1_f32" : "__lanner_tensor_from1_f64";
                auto r = newTemp("tensor.from");
                body.push_back("  " + r + " = call ptr @" + std::string(f) + "(ptr " + p0 + ", i64 " + n + ")");
                return r;
            }
            if (member == "from2F32" || member == "from2F64") {
                auto p0 = arg(0), rows = asU64(1), cols = asU64(2);
                const char* f = member == "from2F32" ? "__lanner_tensor_from2_f32" : "__lanner_tensor_from2_f64";
                auto r = newTemp("tensor.from");
                body.push_back("  " + r + " = call ptr @" + std::string(f) + "(ptr " + p0 + ", i64 " + rows + ", i64 " + cols + ")");
                return r;
            }
            if (member == "clone" || member == "contiguous") {
                auto a = arg(0);
                const char* f = member == "clone" ? "__lanner_tensor_clone" : "__lanner_tensor_contiguous";
                auto r = newTemp("tensor.clone");
                body.push_back("  " + r + " = call ptr @" + std::string(f) + "(ptr " + a + ")");
                return r;
            }
            if (member == "free") { body.push_back("  call void @__lanner_tensor_free(ptr " + arg(0) + ")"); return {}; }
            if (member == "rank" || member == "len") {
                const char* f = member == "rank" ? "__lanner_tensor_rank" : "__lanner_tensor_len";
                auto r = newTemp("tensor.query");
                body.push_back("  " + r + " = call i64 @" + std::string(f) + "(ptr " + arg(0) + ")");
                return r;
            }
            if (member == "dim" || member == "stride") {
                const char* f = member == "dim" ? "__lanner_tensor_dim" : "__lanner_tensor_stride";
                auto r = newTemp("tensor.query");
                body.push_back("  " + r + " = call i64 @" + std::string(f) + "(ptr " + arg(0) + ", i64 " + asU64(1) + ")");
                return r;
            }
            if (member == "dtype") {
                auto r = newTemp("tensor.dtype");
                body.push_back("  " + r + " = call i32 @__lanner_tensor_dtype(ptr " + arg(0) + ")");
                return r;
            }
            if (member == "isContiguous") {
                auto r = newTemp("tensor.contig");
                body.push_back("  " + r + " = call i32 @__lanner_tensor_is_contiguous(ptr " + arg(0) + ")");
                auto b = newTemp("tensor.contig.bool");
                body.push_back("  " + b + " = trunc i32 " + r + " to i1");
                return b;
            }
            if (member == "dataF32" || member == "dataF64") {
                const char* f = member == "dataF32" ? "__lanner_tensor_data_f32" : "__lanner_tensor_data_f64";
                auto r = newTemp("tensor.data");
                body.push_back("  " + r + " = call ptr @" + std::string(f) + "(ptr " + arg(0) + ")");
                return r;
            }
            if (member == "get1" || member == "get2" || member == "get3") {
                const int rank = member.back() - '0';
                std::string args = "ptr " + arg(0);
                for (int i = 0; i < rank; ++i) args += ", i64 " + asU64(static_cast<std::size_t>(1 + i));
                auto r = newTemp("tensor.get");
                body.push_back("  " + r + " = call double @__lanner_tensor_get" + std::to_string(rank) + "(" + args + ")");
                return r;
            }
            if (member == "set1" || member == "set2" || member == "set3") {
                const int rank = member.back() - '0';
                std::string args = "ptr " + arg(0);
                for (int i = 0; i < rank; ++i) args += ", i64 " + asU64(static_cast<std::size_t>(1 + i));
                args += ", double " + asF64(static_cast<std::size_t>(1 + rank));
                body.push_back("  call void @__lanner_tensor_set" + std::to_string(rank) + "(" + args + ")");
                return {};
            }
            if (member == "add" || member == "sub" || member == "mul" || member == "div" || member == "matmul") {
                const char* f = member == "add" ? "__lanner_tensor_add" :
                                member == "sub" ? "__lanner_tensor_sub" :
                                member == "mul" ? "__lanner_tensor_mul" :
                                member == "div" ? "__lanner_tensor_div" : "__lanner_tensor_matmul";
                auto r = newTemp("tensor.op");
                body.push_back("  " + r + " = call ptr @" + std::string(f) + "(ptr " + arg(0) + ", ptr " + arg(1) + ")");
                return r;
            }
            if (member == "scale") {
                auto r = newTemp("tensor.scale");
                body.push_back("  " + r + " = call ptr @__lanner_tensor_scale(ptr " + arg(0) + ", double " + asF64(1) + ")");
                return r;
            }
            if (member == "relu" || member == "sigmoid" || member == "tanh") {
                const char* f = member == "relu" ? "__lanner_tensor_relu" : member == "sigmoid" ? "__lanner_tensor_sigmoid" : "__lanner_tensor_tanh";
                auto r = newTemp("tensor.act");
                body.push_back("  " + r + " = call ptr @" + std::string(f) + "(ptr " + arg(0) + ")");
                return r;
            }
            if (member == "softmax") {
                auto r = newTemp("tensor.softmax");
                body.push_back("  " + r + " = call ptr @__lanner_tensor_softmax(ptr " + arg(0) + ", i64 " + asU64(1) + ")");
                return r;
            }
            if (member == "sum" || member == "mean" || member == "l2Norm" || member == "dot") {
                if (member == "dot") {
                    auto r = newTemp("tensor.dot");
                    body.push_back("  " + r + " = call double @__lanner_tensor_dot(ptr " + arg(0) + ", ptr " + arg(1) + ")");
                    return r;
                }
                const char* f = member == "sum" ? "__lanner_tensor_sum" : member == "mean" ? "__lanner_tensor_mean" : "__lanner_tensor_l2norm";
                auto r = newTemp("tensor.reduce");
                body.push_back("  " + r + " = call double @" + std::string(f) + "(ptr " + arg(0) + ")");
                return r;
            }
            if (member == "argmax") {
                auto r = newTemp("tensor.argmax");
                body.push_back("  " + r + " = call i64 @__lanner_tensor_argmax(ptr " + arg(0) + ", i64 " + asU64(1) + ")");
                return r;
            }
            if (member == "reshape2" || member == "reshape3" || member == "reshape4") {
                const int rank = member.back() - '0';
                std::string args = "ptr " + arg(0);
                for (int i = 0; i < rank; ++i) args += ", i64 " + asU64(static_cast<std::size_t>(1 + i));
                auto r = newTemp("tensor.reshape");
                body.push_back("  " + r + " = call ptr @__lanner_tensor_reshape" + std::to_string(rank) + "(" + args + ")");
                return r;
            }
            if (member == "transpose2") {
                auto r = newTemp("tensor.transpose");
                body.push_back("  " + r + " = call ptr @__lanner_tensor_transpose2(ptr " + arg(0) + ")");
                return r;
            }
            if (member == "slice") {
                auto r = newTemp("tensor.slice");
                body.push_back("  " + r + " = call ptr @__lanner_tensor_slice(ptr " + arg(0) + ", i64 " + asU64(1) + ", i64 " + asU64(2) + ", i64 " + asU64(3) + ", i64 " + asU64(4) + ")");
                return r;
            }
            if (member == "fill") { body.push_back("  call void @__lanner_tensor_fill(ptr " + arg(0) + ", double " + asF64(1) + ")"); return {}; }
            if (member == "conv2d") {
                auto r = newTemp("tensor.conv2d");
                body.push_back("  " + r + " = call ptr @__lanner_tensor_conv2d(ptr " + arg(0) + ", ptr " + arg(1) + ", i64 " + asU64(2) + ", i64 " + asU64(3) + ")");
                return r;
            }
        }
        if (ns == "Grad") {
            if (member == "create") { auto r=newTemp("grad.create"); body.push_back("  " + r + " = call ptr @__lanner_grad_create()"); return r; }
            if (member == "watch") { body.push_back("  call void @__lanner_grad_watch(ptr " + emitExpr(expr->args[0].get()) + ", ptr " + emitExpr(expr->args[1].get()) + ")"); return {}; }
            if (member == "add" || member == "mul" || member == "matmul") {
                const char* f = member == "add" ? "__lanner_grad_add" : member == "mul" ? "__lanner_grad_mul" : "__lanner_grad_matmul";
                auto r = newTemp("grad.op");
                body.push_back("  " + r + " = call ptr @" + std::string(f) + "(ptr " + emitExpr(expr->args[0].get()) + ", ptr " + emitExpr(expr->args[1].get()) + ", ptr " + emitExpr(expr->args[2].get()) + ")");
                return r;
            }
            if (member == "relu" || member == "tanh" || member == "sum") {
                const char* f = member == "relu" ? "__lanner_grad_relu" : member == "tanh" ? "__lanner_grad_tanh" : "__lanner_grad_sum";
                auto r = newTemp("grad.op");
                body.push_back("  " + r + " = call ptr @" + std::string(f) + "(ptr " + emitExpr(expr->args[0].get()) + ", ptr " + emitExpr(expr->args[1].get()) + ")");
                return r;
            }
            if (member == "scale") {
                auto s0 = emitExpr(expr->args[2].get());
                if (exprLLVMType(expr->args[2].get()) == "float") { auto w=newTemp("grad.f64"); body.push_back("  " + w + " = fpext float " + s0 + " to double"); s0=w; }
                auto r = newTemp("grad.scale");
                body.push_back("  " + r + " = call ptr @__lanner_grad_scale(ptr " + emitExpr(expr->args[0].get()) + ", ptr " + emitExpr(expr->args[1].get()) + ", double " + s0 + ")");
                return r;
            }
            if (member == "backward") { body.push_back("  call void @__lanner_grad_backward(ptr " + emitExpr(expr->args[0].get()) + ", ptr " + emitExpr(expr->args[1].get()) + ")"); return {}; }
            if (member == "grad") { auto r=newTemp("grad.get"); body.push_back("  " + r + " = call ptr @__lanner_grad_get(ptr " + emitExpr(expr->args[0].get()) + ", ptr " + emitExpr(expr->args[1].get()) + ")"); return r; }
            if (member == "free") { body.push_back("  call void @__lanner_grad_free(ptr " + emitExpr(expr->args[0].get()) + ")"); return {}; }
        }
        if (ns == "Mobile") {
            if (member == "log") { body.push_back("  call void @__lanner_mobile_log(ptr " + emitExpr(expr->args[0].get()) + ")"); return {}; }
            if (member == "platform") { auto r=newTemp("mobile.platform"); body.push_back("  "+r+" = call ptr @__lanner_mobile_platform()"); return r; }
            if (member == "osVersion") { auto r=newTemp("mobile.os_version"); body.push_back("  "+r+" = call ptr @__lanner_mobile_os_version()"); return r; }
            if (member == "isSimulator") { auto r=newTemp("mobile.simulator"); body.push_back("  "+r+" = call i32 @__lanner_mobile_is_simulator()"); auto b=newTemp("mobile.simulator.bool"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b; }
            if (member == "screenWidth" || member == "screenHeight" || member == "safeAreaTop" || member == "safeAreaBottom" || member == "safeAreaLeft" || member == "safeAreaRight") {
                const char* f = member == "screenWidth" ? "__lanner_mobile_screen_width" : member == "screenHeight" ? "__lanner_mobile_screen_height" : member == "safeAreaTop" ? "__lanner_mobile_safe_top" : member == "safeAreaBottom" ? "__lanner_mobile_safe_bottom" : member == "safeAreaLeft" ? "__lanner_mobile_safe_left" : "__lanner_mobile_safe_right";
                auto r=newTemp("mobile.metric"); body.push_back("  "+r+" = call i32 @"+std::string(f)+"()"); return r;
            }
            if (member == "deviceScale") { auto r=newTemp("mobile.scale"); body.push_back("  "+r+" = call double @__lanner_mobile_device_scale()"); return r; }
            if (member == "openUrl" || member == "clipboardSet") {
                const char* f = member == "openUrl" ? "__lanner_mobile_open_url" : "__lanner_mobile_clipboard_set";
                auto r=newTemp("mobile.bool"); body.push_back("  "+r+" = call i32 @"+std::string(f)+"(ptr "+emitExpr(expr->args[0].get())+")"); auto b=newTemp("mobile.result"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b;
            }
            if (member == "clipboardGet") { auto r=newTemp("mobile.clipboard"); body.push_back("  "+r+" = call ptr @__lanner_mobile_clipboard_get()"); return r; }
            if (member == "vibrate") { auto a=emitExpr(expr->args[0].get()); auto t=exprLLVMType(expr->args[0].get()); if(t!="i32"){auto w=newTemp("mobile.vibrate.ms"); body.push_back("  "+w+" = zext "+t+" "+a+" to i32"); a=w;} auto r=newTemp("mobile.vibrate"); body.push_back("  "+r+" = call i32 @__lanner_mobile_vibrate(i32 "+a+")"); auto b=newTemp("mobile.vibrate.bool"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b; }
            if (member == "requestPermission") { auto a=emitExpr(expr->args[0].get()); auto r=newTemp("mobile.permission"); body.push_back("  "+r+" = call i32 @__lanner_mobile_request_permission(ptr "+a+")"); return r; }
            if (member == "cameraAvailable" || member == "locationAvailable" || member == "bluetoothAvailable") {
                const char* f = member == "cameraAvailable" ? "__lanner_mobile_camera_available" : member == "locationAvailable" ? "__lanner_mobile_location_available" : "__lanner_mobile_bluetooth_available";
                auto r=newTemp("mobile.feature"); body.push_back("  "+r+" = call i32 @"+std::string(f)+"()"); auto b=newTemp("mobile.feature.bool"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b;
            }
            if (member == "appDataPath" || member == "documentsPath" || member == "cachePath") {
                const char* f = member == "appDataPath" ? "__lanner_mobile_app_data_path" : member == "documentsPath" ? "__lanner_mobile_documents_path" : "__lanner_mobile_cache_path";
                auto r=newTemp("mobile.path"); body.push_back("  "+r+" = call ptr @"+std::string(f)+"()"); return r;
            }
        }
        if (ns == "Accel") {
            const char* f = nullptr;
            if (member == "cudaAvailable") f = "__lanner_accel_cuda_available";
            else if (member == "rocmAvailable") f = "__lanner_accel_rocm_available";
            else if (member == "metalAvailable") f = "__lanner_accel_metal_available";
            else if (member == "blasAvailable") f = "__lanner_accel_blas_available";
            if (f) {
                auto r = newTemp("accel.feature"); body.push_back("  " + r + " = call i32 @" + f + "()");
                auto b = newTemp("accel.feature.bool"); body.push_back("  " + b + " = trunc i32 " + r + " to i1"); return b;
            }
            if (member == "backend") { auto r=newTemp("accel.backend"); body.push_back("  " + r + " = call ptr @__lanner_accel_backend()"); return r; }
        }
        if (expr->callee->kind == ExprKind::FieldAccess && expr->callee->target && expr->callee->target->checkedType && expr->callee->target->checkedType->name == "Buffer") {
            const auto bufferMember = expr->callee->field;
            auto b=emitExpr(expr->callee->target.get());
            if(bufferMember=="len"){auto r=newTemp("buffer.len");body.push_back("  "+r+" = call i64 @__lanner_buffer_len(ptr "+b+")"); if(pointerBits==64)return r; auto n=newTemp("buffer.len.narrow");body.push_back("  "+n+" = trunc i64 "+r+" to i32");return n;}
            if(bufferMember=="data"){auto r=newTemp("buffer.data");body.push_back("  "+r+" = call ptr @__lanner_buffer_data(ptr "+b+")");return r;}
            if(bufferMember=="cstr"){auto r=newTemp("buffer.cstr");body.push_back("  "+r+" = call ptr @__lanner_buffer_cstr(ptr "+b+")");return r;}
            if(bufferMember=="free"){body.push_back("  call void @__lanner_buffer_free(ptr "+b+")");return {};}
            if(bufferMember=="appendString"){auto t=emitExpr(expr->args[0].get());auto r=newTemp("buffer.append");body.push_back("  "+r+" = call i32 @__lanner_buffer_append_string(ptr "+b+", ptr "+t+")");auto bb=newTemp("buffer.append.bool");body.push_back("  "+bb+" = trunc i32 "+r+" to i1");return bb;}
            if(bufferMember=="appendBuffer"){auto o=emitExpr(expr->args[0].get());auto r=newTemp("buffer.append");body.push_back("  "+r+" = call i32 @__lanner_buffer_append_buffer(ptr "+b+", ptr "+o+")");auto bb=newTemp("buffer.append.bool");body.push_back("  "+bb+" = trunc i32 "+r+" to i1");return bb;}
        }
        if (ns == "Web") {
            auto callString = [&](const std::string& fn, std::size_t index) {
                const auto a = emitExpr(expr->args[index].get());
                body.push_back("  call void @" + fn + "(ptr " + a + ")");
                return std::string{};
            };
            if (member == "log") return callString("__lanner_web_log", 0);
            if (member == "warn") return callString("__lanner_web_warn", 0);
            if (member == "error") return callString("__lanner_web_error", 0);
            if (member == "nowMs" || member == "random") {
                const auto r = newTemp("web." + member);
                body.push_back("  " + r + " = call double @__lanner_web_" + (member == "nowMs" ? "now_ms" : "random") + "()");
                return r;
            }
            if (member == "setText" || member == "setHtml") {
                const auto a = emitExpr(expr->args[0].get());
                const auto b = emitExpr(expr->args[1].get());
                const auto r = newTemp("web.dom");
                body.push_back("  " + r + " = call i32 @__lanner_web_" + (member == "setText" ? "set_text" : "set_html") + "(ptr " + a + ", ptr " + b + ")");
                return r;
            }
            if (member == "setAttribute") {
                const auto a = emitExpr(expr->args[0].get()); const auto b = emitExpr(expr->args[1].get()); const auto c = emitExpr(expr->args[2].get());
                const auto r = newTemp("web.attr"); body.push_back("  " + r + " = call i32 @__lanner_web_set_attr(ptr " + a + ", ptr " + b + ", ptr " + c + ")"); return r;
            }
            if (member == "addClass" || member == "removeClass") {
                const auto a = emitExpr(expr->args[0].get()); const auto b = emitExpr(expr->args[1].get());
                const auto r = newTemp("web.class"); body.push_back("  " + r + " = call i32 @__lanner_web_" + (member == "addClass" ? "add_class" : "remove_class") + "(ptr " + a + ", ptr " + b + ")"); return r;
            }
            if (member == "remove" || member == "queryCount" || member == "focus") {
                const auto a = emitExpr(expr->args[0].get()); const auto r = newTemp("web.query");
                body.push_back("  " + r + " = call i32 @__lanner_web_" + (member == "remove" ? "remove" : member == "queryCount" ? "query_count" : "focus") + "(ptr " + a + ")"); return r;
            }
            if (member == "setTimeout") {
                const auto cb = emitExpr(expr->args[0].get()); const auto ms = emitExpr(expr->args[1].get()); const auto r = newTemp("web.timeout");
                body.push_back("  " + r + " = call i32 @__lanner_web_set_timeout(ptr " + cb + ", i32 " + ms + ")"); return r;
            }
            if (member == "clearTimeout" || member == "cancelAnimationFrame" || member == "removeEventListener") {
                const auto id = emitExpr(expr->args[0].get());
                if (member == "clearTimeout") body.push_back("  call void @__lanner_web_clear_timeout(i32 " + id + ")");
                else if (member == "cancelAnimationFrame") body.push_back("  call void @__lanner_web_cancel_animation_frame(i32 " + id + ")");
                else body.push_back("  call void @__lanner_web_remove_event_listener(i32 " + id + ")");
                return {};
            }
            if (member == "requestAnimationFrame" || member == "queueMicrotask") {
                const auto cb = emitExpr(expr->args[0].get()); const auto r = newTemp("web.async");
                body.push_back("  " + r + " = call i32 @__lanner_web_" + (member == "requestAnimationFrame" ? "request_animation_frame" : "queue_microtask") + "(ptr " + cb + ")"); return r;
            }
            if (member == "addEventListener") {
                const auto a=emitExpr(expr->args[0].get()); const auto b=emitExpr(expr->args[1].get()); const auto c=emitExpr(expr->args[2].get()); const auto r=newTemp("web.listener");
                body.push_back("  " + r + " = call i32 @__lanner_web_add_event_listener(ptr " + a + ", ptr " + b + ", ptr " + c + ")"); return r;
            }
            if (member == "fetchText") {
                const auto a=emitExpr(expr->args[0].get()); const auto b=emitExpr(expr->args[1].get()); const auto r=newTemp("web.fetch");
                body.push_back("  " + r + " = call i32 @__lanner_web_fetch_text(ptr " + a + ", ptr " + b + ")"); return r;
            }
            if (member == "freeBuffer") { const auto p=emitExpr(expr->args[0].get()); body.push_back("  call void @__lanner_web_buffer_free(ptr " + p + ")"); return {}; }
        }
        if (ns == "Args") {
            if (member=="count") { auto r=newTemp("args.count"); body.push_back("  "+r+" = call i64 @__lanner_process_argc()"); return r; }
            if (member=="at") { auto i=emitExpr(expr->args[0].get()); auto t=exprLLVMType(expr->args[0].get()); if(t!="i64"){auto w=newTemp("args.index");body.push_back("  "+w+" = zext "+t+" "+i+" to i64");i=w;} auto r=newTemp("args.at"); body.push_back("  "+r+" = call ptr @__lanner_process_argv_at(i64 "+i+")"); return r; }
        }
        if (ns == "Env") {
            if(member=="get"){auto n=emitExpr(expr->args[0].get());auto r=newTemp("env.get");body.push_back("  "+r+" = call ptr @__lanner_getenv(ptr "+n+")");return r;}
            if(member=="has"){auto n=emitExpr(expr->args[0].get());auto r=newTemp("env.has.raw");body.push_back("  "+r+" = call i32 @__lanner_env_has(ptr "+n+")");auto b=newTemp("env.has");body.push_back("  "+b+" = trunc i32 "+r+" to i1");return b;}
            if(member=="set"){auto n=emitExpr(expr->args[0].get()),v=emitExpr(expr->args[1].get());auto r=newTemp("env.set");body.push_back("  "+r+" = call i32 @__lanner_set_env_value(ptr "+n+", ptr "+v+")");auto b=newTemp("env.bool");body.push_back("  "+b+" = trunc i32 "+r+" to i1");return b;}
            if(member=="unset"){auto n=emitExpr(expr->args[0].get());auto r=newTemp("env.unset");body.push_back("  "+r+" = call i32 @__lanner_set_env_unset(ptr "+n+")");auto b=newTemp("env.bool");body.push_back("  "+b+" = trunc i32 "+r+" to i1");return b;}
        }
        if (ns == "FS") {
            auto str=[&](std::size_t i){return emitExpr(expr->args[i].get());};
            if(member=="exists"||member=="isFile"||member=="isDir"){auto p=str(0);auto r=newTemp("fs.bool");body.push_back("  "+r+" = call i32 @__lanner_fs_"+member+"(ptr "+p+")");auto b=newTemp("fs.bool.cast");body.push_back("  "+b+" = trunc i32 "+r+" to i1");return b;}
            if(member=="fileSize"){auto p=str(0);auto r=newTemp("fs.size");body.push_back("  "+r+" = call i64 @__lanner_fs_file_size(ptr "+p+")");return r;}
            if(member=="read"){auto p=str(0);auto r=newTemp("fs.read");body.push_back("  "+r+" = call ptr @__lanner_fs_read(ptr "+p+")");return r;}
            if(member=="write"||member=="append"){auto p=str(0),t=str(1);auto r=newTemp("fs.write");body.push_back("  "+r+" = call i32 @__lanner_fs_"+member+"(ptr "+p+", ptr "+t+")");auto b=newTemp("fs.write.bool");body.push_back("  "+b+" = trunc i32 "+r+" to i1");return b;}
            if(member=="writeBuffer"){auto p=str(0),b0=emitExpr(expr->args[1].get());auto d=newTemp("fs.buf.data");body.push_back("  "+d+" = call ptr @__lanner_buffer_data(ptr "+b0+")");auto n=newTemp("fs.buf.len");body.push_back("  "+n+" = call i64 @__lanner_buffer_len(ptr "+b0+")");auto r=newTemp("fs.buf.write");body.push_back("  "+r+" = call i32 @__lanner_fs_write_buffer(ptr "+p+", ptr "+d+", i64 "+n+")");auto ok=newTemp("fs.buf.ok");body.push_back("  "+ok+" = trunc i32 "+r+" to i1");return ok;}
            if(member=="remove"||member=="mkdir"||member=="rmdir"){auto p=str(0);auto r=newTemp("fs.mut");body.push_back("  "+r+" = call i32 @__lanner_fs_"+member+"(ptr "+p+")");auto b=newTemp("fs.mut.bool");body.push_back("  "+b+" = trunc i32 "+r+" to i1");return b;}
            if(member=="rename"||member=="copy"){auto a=str(0),b0=str(1);auto r=newTemp("fs.rename");body.push_back("  "+r+" = call i32 @__lanner_fs_"+member+"(ptr "+a+", ptr "+b0+")");auto ok=newTemp("fs.rename.bool");body.push_back("  "+ok+" = trunc i32 "+r+" to i1");return ok;}
            if(member=="cwd"){auto r=newTemp("fs.cwd");body.push_back("  "+r+" = call ptr @__lanner_fs_cwd()");return r;}
            if(member=="chdir"){auto p=str(0);auto r=newTemp("fs.chdir");body.push_back("  "+r+" = call i32 @__lanner_fs_chdir(ptr "+p+")");auto ok=newTemp("fs.chdir.ok");body.push_back("  "+ok+" = trunc i32 "+r+" to i1");return ok;}
            if(member=="list"){auto p=str(0);auto r=newTemp("fs.list");body.push_back("  "+r+" = call ptr @__lanner_fs_list(ptr "+p+")");return r;}
        }
        if (ns == "Path") {
            if(member=="join"){auto a=emitExpr(expr->args[0].get()),b=emitExpr(expr->args[1].get());auto r=newTemp("path.join");body.push_back("  "+r+" = call ptr @__lanner_path_join(ptr "+a+", ptr "+b+")");return r;}
            if(member=="basename"||member=="dirname"||member=="extension"||member=="stem"||member=="normalize"||member=="absolute"){auto a=emitExpr(expr->args[0].get());auto r=newTemp("path."+member);body.push_back("  "+r+" = call ptr @__lanner_path_"+member+"(ptr "+a+")");return r;}
            if(member=="isAbsolute"){auto a=emitExpr(expr->args[0].get());auto r=newTemp("path.abs");body.push_back("  "+r+" = call i32 @__lanner_path_is_absolute(ptr "+a+")");auto b=newTemp("path.abs.bool");body.push_back("  "+b+" = trunc i32 "+r+" to i1");return b;}
        }
        if (ns == "Regex") {
            if(member=="compile"){auto p=emitExpr(expr->args[0].get());auto r=newTemp("regex.compile");body.push_back("  "+r+" = call ptr @__lanner_regex_compile(ptr "+p+")");return r;}
            if(member=="isMatch"){auto h=emitExpr(expr->args[0].get()),t=emitExpr(expr->args[1].get());auto r=newTemp("regex.match");body.push_back("  "+r+" = call i32 @__lanner_regex_match(ptr "+h+", ptr "+t+")");auto b=newTemp("regex.match.bool");body.push_back("  "+b+" = trunc i32 "+r+" to i1");return b;}
            if(member=="find"){auto h=emitExpr(expr->args[0].get()),t=emitExpr(expr->args[1].get());auto r=newTemp("regex.find");body.push_back("  "+r+" = call i64 @__lanner_regex_find(ptr "+h+", ptr "+t+")");return r;}
            if(member=="free"){auto h=emitExpr(expr->args[0].get());body.push_back("  call void @__lanner_regex_free(ptr "+h+")");return {};}
        }
        if (ns == "Shell") {
            if(member=="run"){auto c=emitExpr(expr->args[0].get());auto r=newTemp("shell.run");body.push_back("  "+r+" = call i32 @__lanner_process_run(ptr "+c+")");return r;}
            if(member=="output"){auto c=emitExpr(expr->args[0].get());auto r=newTemp("shell.output");body.push_back("  "+r+" = call ptr @__lanner_process_output(ptr "+c+")");return r;}
            if(member=="which"){auto c=emitExpr(expr->args[0].get());auto r=newTemp("shell.which");body.push_back("  "+r+" = call ptr @__lanner_shell_which(ptr "+c+")");return r;}
        }
        if (ns == "String") {
            if(member=="parseInt"){auto a=emitExpr(expr->args[0].get());auto r=newTemp("str.parseint");body.push_back("  "+r+" = call i64 @__lanner_string_parse_i64(ptr "+a+")");return r;}
            if(member=="parseFloat"){auto a=emitExpr(expr->args[0].get());auto r=newTemp("str.parsefloat");body.push_back("  "+r+" = call double @__lanner_string_parse_f64(ptr "+a+")");return r;}
        }
        if (ns == "Stdin") {
            if (member == "hasInput") { auto r = newTemp("stdin.ready"); body.push_back("  " + r + " = call i32 @__lanner_stdin_has_input()"); const auto b = newTemp("stdin.bool"); body.push_back("  " + b + " = trunc i32 " + r + " to i1"); return b; }
            if (member == "readLine") { auto r = newTemp("stdin.line"); body.push_back("  " + r + " = call ptr @__lanner_stdin_read_line()"); return r; }
        }
        if (ns == "Clock") {
            if (member == "monotonicNanos") { auto r=newTemp("clock.now"); body.push_back("  "+r+" = call i64 @__lanner_clock_monotonic_nanos()"); return r; }
            if (member == "sleepNanos") { auto a=emitExpr(expr->args[0].get()); body.push_back("  call void @__lanner_clock_sleep_nanos(i64 "+a+")"); return ""; }
            if (member == "deadlineAfterNanos") { auto a=emitExpr(expr->args[0].get()); auto n=newTemp("clock.deadline"); body.push_back("  "+n+" = call i64 @__lanner_clock_monotonic_nanos()"); auto r=newTemp("clock.deadline.add"); body.push_back("  "+r+" = add i64 "+n+", "+a); return r; }
            if (member == "expired") { auto d=emitExpr(expr->args[0].get()); auto n=newTemp("clock.expired.now"); body.push_back("  "+n+" = call i64 @__lanner_clock_monotonic_nanos()"); auto r=newTemp("clock.expired"); body.push_back("  "+r+" = icmp uge i64 "+n+", "+d); return r; }
            if (member == "remainingNanos") { auto d=emitExpr(expr->args[0].get()); auto n=newTemp("clock.rem.now"); body.push_back("  "+n+" = call i64 @__lanner_clock_monotonic_nanos()"); auto nonneg=newTemp("clock.rem.nonneg"); body.push_back("  "+nonneg+" = icmp ult i64 "+n+", "+d); auto delta=newTemp("clock.rem.delta"); body.push_back("  "+delta+" = sub i64 "+d+", "+n); auto r=newTemp("clock.rem"); body.push_back("  "+r+" = select i1 "+nonneg+", i64 "+delta+", i64 0"); return r; }
        }
        if (ns == "Thread") {
            if (member == "hardwareConcurrency") { auto r=newTemp("thread.hw"); body.push_back("  "+r+" = call i64 @__lanner_thread_hardware_concurrency()"); return r; }
            if (member == "yield") { body.push_back("  call void @__lanner_thread_yield()"); return ""; }
            if (member == "spawn") {
                if (expr->args.size()!=1 || expr->args[0]->kind!=ExprKind::Identifier) unsupported("invalid Thread.spawn target", expr->line);
                const auto r = newTemp("thread.spawn");
                body.push_back("  "+r+" = call ptr @__lanner_thread_spawn(ptr @__lanner_thread_entry_"+expr->args[0]->strValue+")");
                return r;
            }
        }
        if (ns == "Net") {
            auto asI32 = [&](const Expr* a, const std::string& tag) {
                auto v = emitExpr(a); auto t = exprLLVMType(a); if (t == "i32") return v; auto r = newTemp(tag); body.push_back("  " + r + " = zext " + t + " " + v + " to i32"); return r;
            };
            auto asI64 = [&](const Expr* a, const std::string& tag) {
                auto v = emitExpr(a); auto t = exprLLVMType(a); if (t == "i64") return v; auto r = newTemp(tag); body.push_back("  " + r + " = zext " + t + " " + v + " to i64"); return r;
            };
            if (member == "tcpConnect") { auto h=emitExpr(expr->args[0].get()); auto p=asI32(expr->args[1].get(),"net.port"); auto tm=emitExpr(expr->args[2].get()); auto r=newTemp("net.connect"); body.push_back("  "+r+" = call ptr @__lanner_net_tcp_connect(ptr "+h+", i32 "+p+", i32 "+tm+")"); return r; }
            if (member == "tcpListen") { auto h=emitExpr(expr->args[0].get()); auto p=asI32(expr->args[1].get(),"net.port"); auto b=emitExpr(expr->args[2].get()); auto r=newTemp("net.listen"); body.push_back("  "+r+" = call ptr @__lanner_net_tcp_listen(ptr "+h+", i32 "+p+", i32 "+b+")"); return r; }
            if (member == "accept") { auto s=emitExpr(expr->args[0].get()); auto r=newTemp("net.accept"); body.push_back("  "+r+" = call ptr @__lanner_net_accept(ptr "+s+")"); return r; }
            if (member == "close") { auto s=emitExpr(expr->args[0].get()); body.push_back("  call void @__lanner_net_close(ptr "+s+")"); return {}; }
            if (member == "send") { auto s=emitExpr(expr->args[0].get()); auto p=emitExpr(expr->args[1].get()); auto n=asI64(expr->args[2].get(),"net.send.len"); auto r=newTemp("net.send"); body.push_back("  "+r+" = call i64 @__lanner_net_send(ptr "+s+", ptr "+p+", i64 "+n+")"); if(pointerBits==64)return r; auto q=newTemp("net.send.narrow"); body.push_back("  "+q+" = trunc i64 "+r+" to i32"); return q; }
            if (member == "recv") { auto s=emitExpr(expr->args[0].get()); auto p=emitExpr(expr->args[1].get()); auto n=asI64(expr->args[2].get(),"net.recv.len"); auto r=newTemp("net.recv"); body.push_back("  "+r+" = call i64 @__lanner_net_recv(ptr "+s+", ptr "+p+", i64 "+n+")"); if(pointerBits==64)return r; auto q=newTemp("net.recv.narrow"); body.push_back("  "+q+" = trunc i64 "+r+" to i32"); return q; }
            if (member == "sendString") { auto s=emitExpr(expr->args[0].get()); auto t=emitExpr(expr->args[1].get()); auto r=newTemp("net.send.str"); body.push_back("  "+r+" = call i64 @__lanner_net_send_string(ptr "+s+", ptr "+t+")"); if(pointerBits==64)return r; auto q=newTemp("net.send.str.narrow"); body.push_back("  "+q+" = trunc i64 "+r+" to i32"); return q; }
            if (member == "setNonblocking" || member == "tcpNoDelay") { auto s=emitExpr(expr->args[0].get()); auto en=emitExpr(expr->args[1].get()); auto r=newTemp("net.bool"); const char* f=member=="setNonblocking"?"__lanner_net_set_nonblocking":"__lanner_net_tcp_nodelay"; body.push_back("  "+r+" = call i32 @"+std::string(f)+"(ptr "+s+", i1 "+en+")"); auto b=newTemp("net.bool.cast"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b; }
            if (member == "poll") { auto s=emitExpr(expr->args[0].get()); auto ev=emitExpr(expr->args[1].get()); auto tm=emitExpr(expr->args[2].get()); auto r=newTemp("net.poll"); body.push_back("  "+r+" = call i32 @__lanner_net_poll(ptr "+s+", i32 "+ev+", i32 "+tm+")"); return r; }
            if (member == "shutdown") { auto s=emitExpr(expr->args[0].get()); auto how=emitExpr(expr->args[1].get()); auto r=newTemp("net.shutdown"); body.push_back("  "+r+" = call i32 @__lanner_net_shutdown(ptr "+s+", i32 "+how+")"); return r; }
            if (member == "lastError") { auto r=newTemp("net.err"); body.push_back("  "+r+" = call i32 @__lanner_net_last_error()"); return r; }
            if (member == "errorString") { auto r=newTemp("net.err.str"); body.push_back("  "+r+" = call ptr @__lanner_net_error_string()"); return r; }
            if (member == "udpOpen") { auto r=newTemp("udp.open"); body.push_back("  "+r+" = call ptr @__lanner_net_udp_open()"); return r; }
            if (member == "udpBind") { auto s=emitExpr(expr->args[0].get()); auto h=emitExpr(expr->args[1].get()); auto p=asI32(expr->args[2].get(),"udp.port"); auto r=newTemp("udp.bind"); body.push_back("  "+r+" = call i32 @__lanner_net_udp_bind(ptr "+s+", ptr "+h+", i32 "+p+")"); auto b=newTemp("udp.bind.bool"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b; }
            if (member == "udpSendTo") { auto s=emitExpr(expr->args[0].get()); auto h=emitExpr(expr->args[1].get()); auto p=asI32(expr->args[2].get(),"udp.port"); auto d=emitExpr(expr->args[3].get()); auto n=asI64(expr->args[4].get(),"udp.len"); auto r=newTemp("udp.send"); body.push_back("  "+r+" = call i64 @__lanner_net_udp_send_to(ptr "+s+", ptr "+h+", i32 "+p+", ptr "+d+", i64 "+n+")"); if(pointerBits==64)return r; auto q=newTemp("udp.send.narrow"); body.push_back("  "+q+" = trunc i64 "+r+" to i32"); return q; }
            if (member == "udpRecv") { auto s=emitExpr(expr->args[0].get()); auto d=emitExpr(expr->args[1].get()); auto n=asI64(expr->args[2].get(),"udp.recv.len"); auto r=newTemp("udp.recv"); body.push_back("  "+r+" = call i64 @__lanner_net_udp_recv(ptr "+s+", ptr "+d+", i64 "+n+")"); if(pointerBits==64)return r; auto q=newTemp("udp.recv.narrow"); body.push_back("  "+q+" = trunc i64 "+r+" to i32"); return q; }
            if (member == "localPort" || member == "peerPort") { auto s=emitExpr(expr->args[0].get()); auto r=newTemp("net.port"); body.push_back("  "+r+" = call i32 @__lanner_net_"+(member=="localPort"?std::string("local_port"):std::string("peer_port"))+"(ptr "+s+")"); auto q=newTemp("net.port.u16"); body.push_back("  "+q+" = trunc i32 "+r+" to i16"); return q; }
        }
        if (ns == "Poller") {
            auto asI32 = [&](const Expr* a, const std::string& tag) { auto v=emitExpr(a); auto t=exprLLVMType(a); if(t=="i32") return v; auto r=newTemp(tag); body.push_back("  "+r+" = zext "+t+" "+v+" to i32"); return r; };
            if (member=="create") { auto r=newTemp("poller"); body.push_back("  "+r+" = call ptr @__lanner_poller_create()"); return r; }
            if (member=="add") { auto p=emitExpr(expr->args[0].get()); auto sck=emitExpr(expr->args[1].get()); auto ev=asI32(expr->args[2].get(),"poller.ev"); auto r=newTemp("poller.add"); body.push_back("  "+r+" = call i32 @__lanner_poller_add(ptr "+p+", ptr "+sck+", i32 "+ev+")"); auto b=newTemp("poller.add.bool"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b; }
            if (member=="remove") { auto p=emitExpr(expr->args[0].get()); auto sck=emitExpr(expr->args[1].get()); auto r=newTemp("poller.remove"); body.push_back("  "+r+" = call i32 @__lanner_poller_remove(ptr "+p+", ptr "+sck+")"); auto b=newTemp("poller.remove.bool"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b; }
            if (member=="wait") { auto p=emitExpr(expr->args[0].get()); auto tm=emitExpr(expr->args[1].get()); auto r=newTemp("poller.wait"); body.push_back("  "+r+" = call i32 @__lanner_poller_wait(ptr "+p+", i32 "+tm+")"); return r; }
            if (member=="count") { auto p=emitExpr(expr->args[0].get()); auto r=newTemp("poller.count"); body.push_back("  "+r+" = call i32 @__lanner_poller_count(ptr "+p+")"); return r; }
            if (member=="eventSocket") { auto p=emitExpr(expr->args[0].get()); auto i=asI32(expr->args[1].get(),"poller.index"); auto r=newTemp("poller.sock"); body.push_back("  "+r+" = call ptr @__lanner_poller_event_socket(ptr "+p+", i32 "+i+")"); return r; }
            if (member=="eventMask") { auto p=emitExpr(expr->args[0].get()); auto i=asI32(expr->args[1].get(),"poller.index"); auto r=newTemp("poller.mask"); body.push_back("  "+r+" = call i32 @__lanner_poller_event_mask(ptr "+p+", i32 "+i+")"); return r; }
            if (member=="readEvents" || member=="writeEvents" || member=="errorEvents") { auto r=newTemp("poller.events"); body.push_back("  "+r+" = call i32 @__lanner_poller_"+(member=="readEvents"?std::string("read_events"):member=="writeEvents"?std::string("write_events"):std::string("error_events"))+"()"); return r; }
            if (member=="destroy") { auto p=emitExpr(expr->args[0].get()); body.push_back("  call void @__lanner_poller_destroy(ptr "+p+")"); return {}; }
        }
        if (ns == "Mutex" || ns == "RwLock" || ns == "Condvar" || ns == "Semaphore") {
            if (member=="create") {
                if(ns=="Mutex"){auto r=newTemp("mutex");body.push_back("  "+r+" = call ptr @__lanner_mutex_create()");return r;}
                if(ns=="RwLock"){auto r=newTemp("rwlock");body.push_back("  "+r+" = call ptr @__lanner_rwlock_create()");return r;}
                if(ns=="Condvar"){auto r=newTemp("condvar");body.push_back("  "+r+" = call ptr @__lanner_condvar_create()");return r;}
                auto v=emitExpr(expr->args[0].get()); auto r=newTemp("semaphore"); body.push_back("  "+r+" = call ptr @__lanner_semaphore_create(i32 "+v+")"); return r;
            }
            if(ns=="Mutex"){auto m=emitExpr(expr->args[0].get()); if(member=="lock"||member=="unlock"||member=="destroy"){const char* f=member=="lock"?"__lanner_mutex_lock":member=="unlock"?"__lanner_mutex_unlock":"__lanner_mutex_destroy";body.push_back("  call void @"+std::string(f)+"(ptr "+m+")");return {};} if(member=="tryLock"){auto r=newTemp("mutex.try");body.push_back("  "+r+" = call i32 @__lanner_mutex_try_lock(ptr "+m+")");auto b=newTemp("mutex.try.bool");body.push_back("  "+b+" = trunc i32 "+r+" to i1");return b;}}
            if(ns=="RwLock"){auto m=emitExpr(expr->args[0].get()); if(member=="readLock"||member=="writeLock"||member=="destroy"){const char* f=member=="readLock"?"__lanner_rwlock_read_lock":member=="writeLock"?"__lanner_rwlock_write_lock":"__lanner_rwlock_destroy";body.push_back("  call void @"+std::string(f)+"(ptr "+m+")");return {};} if(member=="tryReadLock"||member=="tryWriteLock"){const char* f=member=="tryReadLock"?"__lanner_rwlock_try_read_lock":"__lanner_rwlock_try_write_lock";auto r=newTemp("rw.try");body.push_back("  "+r+" = call i32 @"+std::string(f)+"(ptr "+m+")");auto b=newTemp("rw.try.bool");body.push_back("  "+b+" = trunc i32 "+r+" to i1");return b;} if(member=="unlock"){auto w=emitExpr(expr->args[1].get());body.push_back("  call void @__lanner_rwlock_unlock(ptr "+m+", i1 "+w+")");return {};}}
            if(ns=="Condvar"){auto c=emitExpr(expr->args[0].get()); if(member=="wait"){auto m=emitExpr(expr->args[1].get());auto tm=emitExpr(expr->args[2].get());auto r=newTemp("cond.wait");body.push_back("  "+r+" = call i32 @__lanner_condvar_wait(ptr "+c+", ptr "+m+", i32 "+tm+")");auto b=newTemp("cond.wait.bool");body.push_back("  "+b+" = trunc i32 "+r+" to i1");return b;} const char* f=member=="signal"?"__lanner_condvar_signal":member=="broadcast"?"__lanner_condvar_broadcast":"__lanner_condvar_destroy";body.push_back("  call void @"+std::string(f)+"(ptr "+c+")");return {};}
            if(ns=="Semaphore"){auto sem=emitExpr(expr->args[0].get()); if(member=="wait"){auto tm=emitExpr(expr->args[1].get());auto r=newTemp("sem.wait");body.push_back("  "+r+" = call i32 @__lanner_semaphore_wait(ptr "+sem+", i32 "+tm+")");auto b=newTemp("sem.wait.bool");body.push_back("  "+b+" = trunc i32 "+r+" to i1");return b;} if(member=="tryWait"){auto r=newTemp("sem.try");body.push_back("  "+r+" = call i32 @__lanner_semaphore_try_wait(ptr "+sem+")");auto b=newTemp("sem.try.bool");body.push_back("  "+b+" = trunc i32 "+r+" to i1");return b;} const char* f=member=="post"?"__lanner_semaphore_post":"__lanner_semaphore_destroy";body.push_back("  call void @"+std::string(f)+"(ptr "+sem+")");return {};}
        }
        if (ns == "Buffer") {
            if (member == "new") { auto n=emitExpr(expr->args[0].get()); auto t=exprLLVMType(expr->args[0].get()); if(t!="i64"){auto w=newTemp("buffer.capacity");body.push_back("  "+w+" = zext "+t+" "+n+" to i64");n=w;} auto r=newTemp("buffer.new"); body.push_back("  "+r+" = call ptr @__lanner_buffer_new(i64 "+n+")"); return r; }
            if (member == "fromString") { auto t=emitExpr(expr->args[0].get()); auto r=newTemp("buffer.from_string"); body.push_back("  "+r+" = call ptr @__lanner_buffer_from_string(ptr "+t+")"); return r; }
        }
        if (ns == "Process") {
            if(member=="run"){auto c=emitExpr(expr->args[0].get());auto r=newTemp("process.run");body.push_back("  "+r+" = call i32 @__lanner_process_run(ptr "+c+")");return r;}
            if(member=="spawn"){auto c=emitExpr(expr->args[0].get());auto r=newTemp("process.spawn");body.push_back("  "+r+" = call ptr @__lanner_process_spawn(ptr "+c+")");return r;}
            if(member=="wait"){auto h=emitExpr(expr->args[0].get());auto r=newTemp("process.wait");body.push_back("  "+r+" = call i32 @__lanner_process_wait(ptr "+h+")");return r;}
            if(member=="pid"){auto h=emitExpr(expr->args[0].get());auto r=newTemp("process.pid");body.push_back("  "+r+" = call i64 @__lanner_process_pid(ptr "+h+")");return r;}
            if(member=="terminate"){auto h=emitExpr(expr->args[0].get());auto r=newTemp("process.terminate");body.push_back("  "+r+" = call i32 @__lanner_process_terminate(ptr "+h+")");auto b=newTemp("process.terminate.bool");body.push_back("  "+b+" = trunc i32 "+r+" to i1");return b;}
            if(member=="argCount"){auto r=newTemp("process.argc");body.push_back("  "+r+" = call i64 @__lanner_process_argc()");return r;}
            if(member=="arg"){auto i=emitExpr(expr->args[0].get());auto t=exprLLVMType(expr->args[0].get());if(t!="i64"){auto w=newTemp("process.arg.index");body.push_back("  "+w+" = zext "+t+" "+i+" to i64");i=w;}auto r=newTemp("process.argv");body.push_back("  "+r+" = call ptr @__lanner_process_argv_at(i64 "+i+")");return r;}
            if(member=="output"){auto c=emitExpr(expr->args[0].get());auto r=newTemp("process.output");body.push_back("  "+r+" = call ptr @__lanner_process_output(ptr "+c+")");return r;}
            if(member=="setEnv"){auto n=emitExpr(expr->args[0].get());auto v=emitExpr(expr->args[1].get());auto r=newTemp("process.env");body.push_back("  "+r+" = call i32 @__lanner_set_env(ptr "+n+", ptr "+v+")");auto b=newTemp("process.env.bool");body.push_back("  "+b+" = trunc i32 "+r+" to i1");return b;}
        }
        if (ns == "Http") {
            if(member=="get"){auto u=emitExpr(expr->args[0].get());auto tm=emitExpr(expr->args[1].get());auto r=newTemp("http.get");body.push_back("  "+r+" = call ptr @__lanner_http_get(ptr "+u+", i32 "+tm+")");return r;}
            if(member=="post"){auto u=emitExpr(expr->args[0].get());auto b=emitExpr(expr->args[1].get());auto tm=emitExpr(expr->args[2].get());auto r=newTemp("http.post");body.push_back("  "+r+" = call ptr @__lanner_http_post(ptr "+u+", ptr "+b+", i32 "+tm+")");return r;}
            if(member=="status"){auto r=newTemp("http.lanatus");body.push_back("  "+r+" = call i32 @__lanner_http_status()");return r;}
        }
        if (ns == "Json") {
            if(member=="validate"){auto j=emitExpr(expr->args[0].get());auto r=newTemp("json.valid");body.push_back("  "+r+" = call i32 @__lanner_json_validate(ptr "+j+")");auto b=newTemp("json.valid.bool");body.push_back("  "+b+" = trunc i32 "+r+" to i1");return b;}
            if(member=="quote"||member=="int"||member=="float"||member=="bool"||member=="nullValue"){
                if(member=="nullValue"){auto r=newTemp("json.null");body.push_back("  "+r+" = call ptr @__lanner_json_null()");return r;}
                auto a=emitExpr(expr->args[0].get()); auto r=newTemp("json.value");
                if(member=="quote") body.push_back("  "+r+" = call ptr @__lanner_json_quote(ptr "+a+")");
                else if(member=="int") { auto t=exprLLVMType(expr->args[0].get()); if(t!="i64"){auto w=newTemp("json.int64"); body.push_back("  "+w+" = sext "+t+" "+a+" to i64");a=w;} body.push_back("  "+r+" = call ptr @__lanner_json_int(i64 "+a+")"); }
                else if(member=="float") { body.push_back("  "+r+" = call ptr @__lanner_json_float(double "+a+")"); }
                else { body.push_back("  "+r+" = call ptr @__lanner_json_bool(i1 "+a+")"); }
                return r;
            }
        }
        if (ns == "Cpu") {
            if (member == "hasAvx2" || member == "hasAvx512" || member == "hasSse42" || member == "hasBmi2" || member == "hasPopcnt") {
                const std::string sym = member == "hasAvx2" ? "avx2" : member == "hasAvx512" ? "avx512" : member == "hasSse42" ? "sse42" : member == "hasBmi2" ? "bmi2" : "popcnt";
                const auto r = newTemp("cpu.feature");
                body.push_back("  " + r + " = call i32 @__lanner_cpu_has_" + sym + "()");
                const auto b = newTemp("cpu.feature.bool");
                body.push_back("  " + b + " = trunc i32 " + r + " to i1");
                return b;
            }
            if (member == "rdtsc") { auto r=newTemp("rdtsc"); body.push_back("  "+r+" = call i64 @llvm.readcyclecounter()"); return r; }
            if (member == "popcount" || member == "ctz" || member == "clz") {
                const auto a=emitExpr(expr->args[0].get()); auto r=newTemp("cpu.int");
                const char* intrinsic = member == "popcount" ? "llvm.ctpop.i64" : member == "ctz" ? "llvm.cttz.i64" : "llvm.ctlz.i64";
                if (member == "popcount") {
                    body.push_back("  " + r + " = call i64 @" + intrinsic + "(i64 " + a + ")");
                } else {
                    body.push_back("  " + r + " = call i64 @" + intrinsic + "(i64 " + a + ", i1 false)");
                }
                const auto narrowed = newTemp("cpu.int32");
                body.push_back("  " + narrowed + " = trunc i64 " + r + " to i32");
                return narrowed;
            }
            if (member == "bswap") { auto a=emitExpr(expr->args[0].get()); auto r=newTemp("cpu.bswap"); body.push_back("  "+r+" = call i64 @llvm.bswap.i64(i64 "+a+")"); return r; }
            if (member == "pext" || member == "pdep") { auto a=emitExpr(expr->args[0].get()); auto b=emitExpr(expr->args[1].get()); auto r=newTemp("cpu.bmi2"); body.push_back("  "+r+" = call i64 @llvm.x86.bmi."+member+".64(i64 "+a+", i64 "+b+")"); return r; }
            if (member == "prefetch") { auto a=emitExpr(expr->args[0].get()); body.push_back("  call void @llvm.prefetch(ptr "+a+", i32 0, i32 3, i32 1)"); return ""; }
            if (member == "loadV128" || member == "loadV256" || member == "loadV512") { auto a=emitExpr(expr->args[0].get()); const std::string ty=member=="loadV128"?"<2 x i64>":member=="loadV256"?"<4 x i64>":"<8 x i64>"; auto r=newTemp("simd.load"); body.push_back("  "+r+" = load "+ty+", ptr "+a); return r; }
            if (member == "zeroV128" || member == "zeroV256" || member == "zeroV512") { const std::string ty=member=="zeroV128"?"<2 x i64>":member=="zeroV256"?"<4 x i64>":"<8 x i64>"; return "zeroinitializer"; }
            if (member == "storeV128" || member == "storeV256" || member == "storeV512") { auto v=emitExpr(expr->args[0].get()); auto p=emitExpr(expr->args[1].get()); const std::string ty=member=="storeV128"?"<2 x i64>":member=="storeV256"?"<4 x i64>":"<8 x i64>"; body.push_back("  store "+ty+" "+v+", ptr "+p); return ""; }
        }
    }
    if (!expr || expr->callee->kind != ExprKind::Identifier) return {};
    const std::string& name = expr->callee->strValue;
    if (name == "sizeOf" || name == "alignOf" || name == "offsetOf") {
        const auto* valueType = expr->args[0]->checkedType.get();
        if (!valueType) unsupported(name + "() argument has no checked type", expr->line);
        auto plain = cloneType(valueType);
        if (plain->isReference) plain->isReference = false;
        std::string value;
        if (name == "sizeOf") {
            value = "ptrtoint (ptr getelementptr (" + llvmType(plain.get()) + ", ptr null, i64 1) to i" + std::to_string(pointerBits) + ")";
        } else if (name == "alignOf") {
            value = "ptrtoint (ptr getelementptr ({ i8, " + llvmType(plain.get()) + " }, ptr null, i64 1, i32 1) to i" + std::to_string(pointerBits) + ")";
        } else {
            const auto fieldName = expr->args[1]->strValue;
            const auto it = structs.find(plain->name);
            if (it == structs.end()) unsupported("offsetOf() requires a struct type", expr->line);
            std::size_t fieldIndex = 0; bool found = false;
            for (std::size_t i=0;i<it->second->fields.size();++i) if (it->second->fields[i].name == fieldName) { fieldIndex=i; found=true; break; }
            if (!found) unsupported("offsetOf() field does not exist", expr->line);
            value = "ptrtoint (ptr getelementptr (" + llvmType(plain.get()) + ", ptr null, i64 0, i32 " + std::to_string(fieldIndex) + ") to i" + std::to_string(pointerBits) + ")";
        }
        return value;
    }
    if (name == "ptrDiff") {
        auto lhs = emitExpr(expr->args[0].get());
        auto rhs = emitExpr(expr->args[1].get());
        const auto li = newTemp("ptr.diff.l");
        const auto ri = newTemp("ptr.diff.r");
        const auto bytes = newTemp("ptr.diff.bytes");
        const auto result = newTemp("ptr.diff");
        body.push_back("  " + li + " = ptrtoint ptr " + lhs + " to i" + std::to_string(pointerBits));
        body.push_back("  " + ri + " = ptrtoint ptr " + rhs + " to i" + std::to_string(pointerBits));
        body.push_back("  " + bytes + " = sub i" + std::to_string(pointerBits) + " " + li + ", " + ri);
        auto pointee = cloneType(expr->args[0]->checkedType.get());
        pointee->isRawPointer = false; pointee->isMutable = false;
        const auto size = "ptrtoint (ptr getelementptr (" + llvmType(pointee.get()) + ", ptr null, i64 1) to i" + std::to_string(pointerBits) + ")";
        body.push_back("  " + result + " = sdiv i" + std::to_string(pointerBits) + " " + bytes + ", " + size);
        return result;
    }
    if (name == "allocAligned") {
        auto n=emitExpr(expr->args[0].get()); auto a=emitExpr(expr->args[1].get());
        const auto nt=exprLLVMType(expr->args[0].get()), at=exprLLVMType(expr->args[1].get());
        if(nt!="i64"){auto w=newTemp("aligned.size");body.push_back("  "+w+" = zext "+nt+" "+n+" to i64");n=w;}
        if(at!="i64"){auto w=newTemp("aligned.align");body.push_back("  "+w+" = zext "+at+" "+a+" to i64");a=w;}
        auto r=newTemp("alloc.aligned"); body.push_back("  "+r+" = call ptr @lanner_aligned_alloc(i64 "+n+", i64 "+a+")"); return r;
    }
    if (name == "deallocAligned") { auto p=emitExpr(expr->args[0].get()); body.push_back("  call void @lanner_aligned_free(ptr "+p+")"); return {}; }
    if (name == "alloc") { auto n=emitExpr(expr->args[0].get()); const auto nt=exprLLVMType(expr->args[0].get()); if(nt!="i64"){auto w=newTemp("alloc.size");body.push_back("  "+w+" = zext "+nt+" "+n+" to i64");n=w;} auto r=newTemp("alloc"); body.push_back("  "+r+" = call ptr @malloc(i64 "+n+")"); return r; }
    if (name == "realloc") { auto p=emitExpr(expr->args[0].get()); auto n=emitExpr(expr->args[1].get()); const auto nt=exprLLVMType(expr->args[1].get()); if(nt!="i64"){auto w=newTemp("realloc.size");body.push_back("  "+w+" = zext "+nt+" "+n+" to i64");n=w;} auto r=newTemp("realloc"); body.push_back("  "+r+" = call ptr @realloc(ptr "+p+", i64 "+n+")"); return r; }
    if (name == "stackAlloc") {
        auto n = emitExpr(expr->args[0].get());
        const auto nt = exprLLVMType(expr->args[0].get());
        if (nt != "i64") { auto w = newTemp("stack.size"); body.push_back("  " + w + " = zext " + nt + " " + n + " to i64"); n = w; }
        std::string align = "1";
        if (expr->args.size() == 2) align = std::to_string(std::stoull(expr->args[1]->strValue));
        const auto r = newTemp("stack.alloc");
        body.push_back("  " + r + " = alloca i8, i64 " + n + ", align " + align);
        return r;
    }
    if (name == "assume") { auto c = emitExpr(expr->args[0].get()); body.push_back("  call void @llvm.assume(i1 " + c + ")"); return {}; }
    if (name == "trap") { body.push_back("  call void @llvm.trap()"); body.push_back("  unreachable"); blockTerminated = true; return {}; }
    if (name == "dealloc") { auto p=emitExpr(expr->args[0].get()); body.push_back("  call void @free(ptr "+p+")"); return {}; }
    if (name == "volatileLoad") { auto p=emitExpr(expr->args[0].get()); auto ptype=expr->args[0]->checkedType.get(); auto valueType=cloneType(ptype); valueType->isRawPointer=false; valueType->isMutable=false; auto r=newTemp("volatile.load"); body.push_back("  "+r+" = load volatile "+llvmType(valueType.get())+", ptr "+p); return r; }
    if (name == "volatileStore") { auto p=emitExpr(expr->args[0].get()); auto v=emitExpr(expr->args[1].get()); auto ptype=expr->args[0]->checkedType.get(); auto valueType=cloneType(ptype); valueType->isRawPointer=false; valueType->isMutable=false; body.push_back("  store volatile "+llvmType(valueType.get())+" "+v+", ptr "+p); return {}; }
    if (name == "unalignedLoad") { auto p=emitExpr(expr->args[0].get()); auto ptype=expr->args[0]->checkedType.get(); auto valueType=cloneType(ptype); valueType->isRawPointer=false; valueType->isMutable=false; auto r=newTemp("unaligned.load"); body.push_back("  "+r+" = load "+llvmType(valueType.get())+", ptr "+p+", align 1"); return r; }
    if (name == "unalignedStore") { auto p=emitExpr(expr->args[0].get()); auto v=emitExpr(expr->args[1].get()); auto ptype=expr->args[0]->checkedType.get(); auto valueType=cloneType(ptype); valueType->isRawPointer=false; valueType->isMutable=false; body.push_back("  store "+llvmType(valueType.get())+" "+v+", ptr "+p+", align 1"); return {}; }
    if (name == "memset") {
        auto p=emitExpr(expr->args[0].get()); auto v=emitExpr(expr->args[1].get()); auto n=emitExpr(expr->args[2].get());
        const auto nty = exprLLVMType(expr->args[2].get());
        if (nty != "i64") { auto w=newTemp("memset.size"); body.push_back("  "+w+" = zext "+nty+" "+n+" to i64"); n=w; }
        const std::string vty = exprLLVMType(expr->args[1].get());
        if (vty != "i32") { auto wide=newTemp("memset.byte"); body.push_back("  "+wide+" = zext "+vty+" "+v+" to i32"); v=wide; }
        body.push_back("  call ptr @memset(ptr "+p+", i32 "+v+", i64 "+n+")"); return p;
    }
    if (name == "memcpy" || name == "memmove") {
        auto d=emitExpr(expr->args[0].get()); auto ss=emitExpr(expr->args[1].get()); auto n=emitExpr(expr->args[2].get());
        const auto nty = exprLLVMType(expr->args[2].get());
        if (nty != "i64") { auto w=newTemp("mem.size"); body.push_back("  "+w+" = zext "+nty+" "+n+" to i64"); n=w; }
        body.push_back("  call ptr @"+name+"(ptr "+d+", ptr "+ss+", i64 "+n+")"); return d;
    }
    if (name == "memcmp") {
        auto d=emitExpr(expr->args[0].get()); auto ss=emitExpr(expr->args[1].get()); auto n=emitExpr(expr->args[2].get());
        const auto nty = exprLLVMType(expr->args[2].get());
        if (nty != "i64") { auto w=newTemp("memcmp.size"); body.push_back("  "+w+" = zext "+nty+" "+n+" to i64"); n=w; }
        auto r=newTemp("memcmp"); body.push_back("  "+r+" = call i32 @memcmp(ptr "+d+", ptr "+ss+", i64 "+n+")"); return r;
    }
    if (name == "atomicFence") {
        const auto order = expr->args[0]->strValue;
        if (order == "relaxed") return {};
        body.push_back("  fence "+order);
        return {};
    }
    if (name == "compilerFence") {
        body.push_back("  call void asm sideeffect \"\\22\", \"~{memory}\"");
        return {};
    }
    if (name == "unreachable") { body.push_back("  unreachable"); blockTerminated=true; return {}; }
    if (name == "asm" || name == "asmI64" || name == "asmI32" || name == "asmPtr") {
        if (expr->args[0]->kind != ExprKind::StringLit || expr->args[1]->kind != ExprKind::StringLit) unsupported("asm template and constraints must be string literals", expr->line);
        std::string templ=expr->args[0]->strValue;
        for (std::size_t i=0;i<templ.size();++i) { if (templ[i]=='\\') { templ.insert(i,"\\"); ++i; } else if (templ[i]=='\"') { templ.insert(i,"\\"); ++i; } }
        const std::string constraints=expr->args[1]->strValue;
        const std::string asmType = name == "asmI64" ? "i64" : name == "asmI32" ? "i32" : name == "asmPtr" ? "ptr" : "void";
        std::string call="call "+asmType+" asm sideeffect \""+templ+"\", \""+constraints+"\"(";
        for(std::size_t i=2;i<expr->args.size();++i){ if(i>2) call+=", "; call+=exprLLVMType(expr->args[i].get())+" "+emitExpr(expr->args[i].get()); }
        call+=")";
        if(name=="asm") { body.push_back("  "+call); return {}; }
        auto r=newTemp("asm"); body.push_back("  "+r+" = "+call); return r;
    }
    if (name == "readFile") {
        const auto path = emitExpr(expr->args[0].get());
        const auto result = newTemp("readfile");
        body.push_back("  " + result + " = call %LannerDynArray @__lanner_read_file(ptr " + path + ")");
        return result;
    }
    if (name == "writeStdout") {
        const auto text = emitExpr(expr->args[0].get());
        if (isWebTarget()) body.push_back("  call void @__lanner_web_log(ptr " + text + ")");
        else body.push_back("  call i32 @puts(ptr " + text + ")");
        return {};
    }
    if (name == "writeRaw") {
        const auto text = emitExpr(expr->args[0].get());
        if (isWebTarget()) {
            body.push_back("  call void @__lanner_web_log(ptr " + text + ")");
            return {};
        }
        body.push_back("  %raw.len" + std::to_string(labelCounter++) + " = call i64 @strlen(ptr " + text + ")");
        const auto id = std::to_string(labelCounter-1);
        body.push_back("  %raw.out" + id + " = load ptr, ptr @stdout");
        body.push_back("  call i64 @fwrite(ptr " + text + ", i64 1, i64 %raw.len" + id + ", ptr %raw.out" + id + ")");
        return {};
    }
    if (name == "writeIntRaw") {
        const auto value = emitExpr(expr->args[0].get());
        if (isWebTarget()) body.push_back("  call void @__lanner_web_log_i64(i64 " + value + ")");
        else body.push_back("  call i32 (ptr, ...) @printf(ptr @.lanner.print_i64_raw, i64 " + value + ")");
        return {};
    }
    if (name == "writeByteRaw") {
        const auto value = emitExpr(expr->args[0].get());
        if (isWebTarget()) {
            body.push_back("  call void @__lanner_web_log_i64(i64 " + value + ")");
            return {};
        }
        const auto id = std::to_string(labelCounter++);
        body.push_back("  %raw.byte" + id + " = trunc i64 " + value + " to i8");
        body.push_back("  %raw.buf" + id + " = alloca i8, align 1");
        body.push_back("  store i8 %raw.byte" + id + ", ptr %raw.buf" + id);
        body.push_back("  %raw.out" + id + " = load ptr, ptr @stdout");
        body.push_back("  call i64 @fwrite(ptr %raw.buf" + id + ", i64 1, i64 1, ptr %raw.out" + id + ")");
        return {};
    }
    if (name == "print") return emitPrintCall(expr);
    if (name == "printInt") {
        const auto value = emitExpr(expr->args[0].get());
        if (isWebTarget()) body.push_back("  call void @__lanner_web_log_i64(i64 " + value + ")");
        else body.push_back("  call void @__lanner_print_i64(i64 " + value + ")");
        return {};
    }
    if (name == "stringLen") {
        const auto text = emitExpr(expr->args[0].get());
        const auto result = newTemp("strlen");
        body.push_back("  " + result + " = call i64 @strlen(ptr " + text + ")");
        return result;
    }
    if (name == "getEnv") {
        const auto text = emitExpr(expr->args[0].get());
        const auto result = newTemp("getenv");
        body.push_back("  " + result + " = call ptr @__lanner_getenv(ptr " + text + ")");
        return result;
    }
    return {};
}

std::string LLVMCodeGenerator::emitImplicitOptionalWrap(const Expr* expr) {
    if (!expr || !expr->checkedType || !expr->checkedType->isOptional) {
        unsupported("invalid implicit optional conversion", expr ? expr->line : 0);
    }
    auto* mutableExpr = const_cast<Expr*>(expr);
    mutableExpr->implicitOptionalWrap = false;
    const auto optionalType = cloneType(expr->checkedType.get());
    auto payloadType = cloneType(optionalType.get());
    payloadType->isOptional = false;
    auto savedType = std::move(mutableExpr->checkedType);
    mutableExpr->checkedType = cloneType(payloadType.get());
    const auto payload = emitExpr(expr);
    mutableExpr->checkedType = std::move(savedType);
    mutableExpr->implicitOptionalWrap = true;

    const auto resultType = llvmType(optionalType.get());
    const auto tag = newTemp("optional.some.tag");
    const auto tagged = newTemp("optional.some");
    body.push_back("  " + tag + " = insertvalue " + resultType + " zeroinitializer, i1 1, 0");
    body.push_back("  " + tagged + " = insertvalue " + resultType + " " + tag + ", " +
                   llvmType(payloadType.get()) + " " + payload + ", 1");
    return tagged;
}

std::string LLVMCodeGenerator::emitExpr(const Expr* expr) {
    if (!expr) unsupported("null expression", 0);
    if (expr->implicitOptionalWrap) return emitImplicitOptionalWrap(expr);

    switch (expr->kind) {
        case ExprKind::IntLit:
            return normalizeIntLiteral(expr->strValue);

        case ExprKind::FloatLit:
            return llvmFloatLiteral(expr);

        case ExprKind::BoolLit:
            return expr->strValue == "true" ? "1" : "0";

        case ExprKind::NoneLit:
            if (expr->strValue == "null") return "null";
            if (!expr->checkedType || !expr->checkedType->isOptional) {
                unsupported("none has no contextual optional type", expr->line);
            }
            return "zeroinitializer";

        case ExprKind::OkLit:
        case ExprKind::ErrLit: {
            if (!expr->checkedType || expr->checkedType->name != "Result" || expr->checkedType->generics.size() != 2) {
                unsupported("Result literal has no complete Result type", expr->line);
            }
            const auto resultType = llvmType(expr->checkedType.get());
            const bool isOk = expr->kind == ExprKind::OkLit;
            const auto payload = emitExpr(expr->value.get());
            if (expr->value->kind == ExprKind::Identifier &&
                !TypeChecker::isCopyType(expr->checkedType->generics[isOk ? 0 : 1].get())) {
                if (auto* source = lookupLocal(expr->value->strValue)) source->moved = true;
            }
            const auto tagged = newTemp(isOk ? "result.ok.tag" : "result.err.tag");
            const auto completed = newTemp(isOk ? "result.ok" : "result.err");
            body.push_back("  " + tagged + " = insertvalue " + resultType + " zeroinitializer, i1 " +
                           (isOk ? "1" : "0") + ", 0");
            body.push_back("  " + completed + " = insertvalue " + resultType + " " + tagged + ", " +
                           llvmType(expr->checkedType->generics[isOk ? 0 : 1].get()) + " " + payload + ", " +
                           (isOk ? "1" : "2"));
            return completed;
        }

        case ExprKind::StringLit:
            return llvmStringLiteral(expr->strValue);

        case ExprKind::Identifier: {
            if (const auto* comptime = lookupComptime(expr->strValue)) {
                return comptimeLiteral(*comptime);
            }
            const auto* local = lookupLocal(expr->strValue);
            const StaticDecl* staticDecl = nullptr;
            if (!local) {
                auto it = statics.find(expr->strValue);
                if (it != statics.end()) staticDecl = it->second;
            }
            if (!local && !staticDecl) {
                if (functions.count(expr->strValue) && expr->checkedType && expr->checkedType->name == "fn") return "@" + expr->strValue;
                unsupported("unknown value '" + expr->strValue + "'", expr->line);
            }
            if (local && local->moved) unsupported("use of moved local '" + expr->strValue + "'", expr->line);
            const TypeNode* valueType = local ? local->type : staticDecl->type.get();
            const std::string type = llvmType(valueType);
            if (local && local->type->isReference && (!expr->checkedType || !expr->checkedType->isReference)) {
                const auto ptr = newTemp("ref.load");
                body.push_back("  " + ptr + " = load ptr, ptr " + local->addr);
                auto derefType = cloneType(local->type);
                derefType->isReference = false;
                derefType->isMutable = false;
                const auto value = newTemp("deref");
                body.push_back("  " + value + " = load " + llvmType(derefType.get()) + ", ptr " + ptr);
                return value;
            }
            const auto address = local ? local->addr : "@" + expr->strValue;
            const auto value = newTemp("load");
            if (valueType->name == "Atomic" && valueType->generics.size() == 1) {
                body.push_back("  " + value + " = load atomic " + type + ", ptr " + address + " seq_cst, align " + std::to_string(std::max(1, ((valueType->generics[0]->name == "usize" || valueType->generics[0]->name == "isize") ? pointerBits : intBits(valueType->generics[0]->name)) / 8)));
            } else {
                body.push_back("  " + value + " = load " + type + ", ptr " + address);
            }
            return value;
        }

        case ExprKind::IsMatch: {
            if (!expr->left || !expr->left->checkedType || expr->left->checkedType->name != "Result") {
                unsupported("Result match has no Result operand", expr->line);
            }
            const auto resultValue = emitExpr(expr->left.get());
            const auto tag = newTemp("result.tag");
            body.push_back("  " + tag + " = extractvalue " + llvmType(expr->left->checkedType.get()) + " " + resultValue + ", 0");
            return expr->matchKind == "Ok" ? tag : [&]() {
                const auto inverted = newTemp("result.not");
                body.push_back("  " + inverted + " = xor i1 " + tag + ", true");
                return inverted;
            }();
        }

        case ExprKind::FieldAccess: {
            if (expr->target->kind == ExprKind::Identifier) {
                auto it = enums.find(expr->target->strValue);
                if (it != enums.end()) {
                    for (const auto& variant : it->second->variants) {
                        if (variant.name == expr->field) return std::to_string(variant.value);
                    }
                }
            }
            const auto address = emitLValueAddress(expr);
            const auto value = newTemp("field");
            body.push_back("  " + value + " = load " + exprLLVMType(expr) + ", ptr " + address);
            return value;
        }

        case ExprKind::UnaryOp: {
            if (expr->op == "&" || expr->op == "&mut" || expr->op == "&raw") return emitLValueAddress(expr->value.get());
            if (expr->op == "*") {
                const auto ptr = emitExpr(expr->value.get());
                auto pointee = cloneType(expr->value->checkedType.get());
                if (!pointee) unsupported("dereference missing pointer type", expr->line);
                pointee->isRawPointer = false;
                pointee->isMutable = false;
                const auto result = newTemp("raw.deref");
                body.push_back("  " + result + " = load " + llvmType(pointee.get()) + ", ptr " + ptr);
                return result;
            }
            const auto value = emitExpr(expr->value.get());
            const auto type = exprLLVMType(expr->value.get());
            if (expr->op == "+") return value;
            const auto result = newTemp("unary");
            if (expr->op == "!") {
                body.push_back("  " + result + " = xor i1 " + value + ", true");
            } else if (expr->op == "~") {
                body.push_back("  " + result + " = xor " + type + " " + value + ", -1");
            } else if (type == "float" || type == "double") {
                body.push_back("  " + result + " = fneg " + type + " " + value);
            } else {
                body.push_back("  " + result + " = sub " + type + " 0, " + value);
            }
            return result;
        }

        case ExprKind::BinaryOp: {
            const auto& op = expr->op;
            if (op == "&&" || op == "||") {
                const auto lhs = emitExpr(expr->left.get());
                const auto rhsLabel = newLabel("logic.rhs");
                const auto shortLabel = newLabel("logic.short");
                const auto mergeLabel = newLabel("logic.merge");
                if (op == "&&") {
                    body.push_back("  br i1 " + lhs + ", label %" + rhsLabel + ", label %" + shortLabel);
                } else {
                    body.push_back("  br i1 " + lhs + ", label %" + shortLabel + ", label %" + rhsLabel);
                }
                body.push_back(shortLabel + ":");
                body.push_back("  br label %" + mergeLabel);
                body.push_back(rhsLabel + ":");
                const auto rhs = emitExpr(expr->right.get());
                // The RHS may itself contain short-circuit expressions, which means
                // control flow can pass through several nested blocks before reaching
                // the value-producing block. The PHI incoming block must name the
                // actual predecessor of the merge block, not the initial RHS block.
                std::string rhsExitLabel;
                for (auto it = body.rbegin(); it != body.rend(); ++it) {
                    const auto& line = *it;
                    if (!line.empty() && line.back() == ':') {
                        const auto first = line.find_first_not_of(" \t");
                        if (first == 0) {
                            rhsExitLabel = line.substr(0, line.size() - 1);
                            break;
                        }
                    }
                }
                if (rhsExitLabel.empty()) unsupported("internal error: missing short-circuit RHS block", expr->line);
                body.push_back("  br label %" + mergeLabel);
                body.push_back(mergeLabel + ":");
                const auto result = newTemp("logic");
                body.push_back("  " + result + " = phi i1 [ " + (op == "&&" ? "0" : "1") + ", %" +
                               shortLabel + " ], [ " + rhs + ", %" + rhsExitLabel + " ]");
                return result;
            }

            const auto lhs = emitExpr(expr->left.get());
            const auto rhs = emitExpr(expr->right.get());
            const auto* lhsType = expr->left->checkedType.get();
            const auto* rhsType = expr->right->checkedType.get();
            if (lhsType && rhsType && lhsType->isRawPointer) {
                if ((op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") && rhsType->isRawPointer) {
                    const auto result = newTemp("ptr.cmp");
                    const std::string pred = op == "==" ? "eq" : op == "!=" ? "ne" : op == "<" ? "ult" : op == ">" ? "ugt" : op == "<=" ? "ule" : "uge";
                    body.push_back("  " + result + " = icmp " + pred + " ptr " + lhs + ", " + rhs);
                    return result;
                }
                if (op == "+" || op == "-") {
                    const auto indexI64 = normalizeIndexToI64(expr->right.get(), rhs);
                    auto pointee = cloneType(lhsType);
                    pointee->isRawPointer = false;
                    pointee->isMutable = false;
                    const auto address = newTemp("raw.ptr.arith");
                    std::string offset = indexI64;
                    if (op == "-") {
                        const auto neg = newTemp("raw.ptr.neg");
                        body.push_back("  " + neg + " = sub i64 0, " + indexI64);
                        offset = neg;
                    }
                    body.push_back("  " + address + " = getelementptr " + llvmType(pointee.get()) + ", ptr " + lhs + ", i64 " + offset);
                    return address;
                }
            }
            if (lhsType && rhsType && isIntegerLLVM(exprLLVMType(expr->left.get())) && rhsType->isRawPointer && op == "+") {
                const auto indexI64 = normalizeIndexToI64(expr->left.get(), lhs);
                auto pointee = cloneType(rhsType);
                pointee->isRawPointer = false;
                pointee->isMutable = false;
                const auto address = newTemp("raw.ptr.arith");
                body.push_back("  " + address + " = getelementptr " + llvmType(pointee.get()) + ", ptr " + rhs + ", i64 " + indexI64);
                return address;
            }
            std::string type = exprLLVMType(expr->left.get());
            if (expr->left->kind == ExprKind::IntLit && type == "i32") type = exprLLVMType(expr->right.get());
            const bool unsignedOp = expr->left->checkedType && isUnsignedType(expr->left->checkedType.get());
            const auto result = newTemp("bin");

            if (type == "float" || type == "double") {
                if (op == "+" || op == "-" || op == "*" || op == "/") {
                    const std::string ir = op == "+" ? "fadd" : op == "-" ? "fsub" : op == "*" ? "fmul" : "fdiv";
                    body.push_back("  " + result + " = " + ir + " " + type + " " + lhs + ", " + rhs);
                    return result;
                }
                if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") {
                    const std::string pred = op == "==" ? "oeq" : op == "!=" ? "one" : op == "<" ? "olt" :
                                             op == ">" ? "ogt" : op == "<=" ? "ole" : "oge";
                    body.push_back("  " + result + " = fcmp " + pred + " " + type + " " + lhs + ", " + rhs);
                    return result;
                }
            }

            if (type == "ptr" && (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=")) {
                std::string pred = op == "==" ? "eq" : op == "!=" ? "ne" : op == "<" ? "ult" :
                                   op == ">" ? "ugt" : op == "<=" ? "ule" : "uge";
                body.push_back("  " + result + " = icmp " + pred + " ptr " + lhs + ", " + rhs);
                return result;
            }

            if (isIntegerLLVM(type)) {
                std::string ir;
                if (op == "+") ir = "add";
                else if (op == "-") ir = "sub";
                else if (op == "*") ir = "mul";
                else if (op == "/") ir = unsignedOp ? "udiv" : "sdiv";
                else if (op == "%") ir = unsignedOp ? "urem" : "srem";
                else if (op == "&") ir = "and";
                else if (op == "|") ir = "or";
                else if (op == "^") ir = "xor";
                else if (op == "<<") ir = "shl";
                else if (op == ">>") ir = unsignedOp ? "lshr" : "ashr";
                if (!ir.empty()) {
                    body.push_back("  " + result + " = " + ir + " " + type + " " + lhs + ", " + rhs);
                    return result;
                }
                if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") {
                    const std::string pred = op == "==" ? "eq" : op == "!=" ? "ne" : op == "<" ? (unsignedOp ? "ult" : "slt") :
                                             op == ">" ? (unsignedOp ? "ugt" : "sgt") : op == "<=" ? (unsignedOp ? "ule" : "sle") :
                                             (unsignedOp ? "uge" : "sge");
                    body.push_back("  " + result + " = icmp " + pred + " " + type + " " + lhs + ", " + rhs);
                    return result;
                }
            }
            unsupported("operator '" + op + "' is not lowered", expr->line);
        }

        case ExprKind::ArenaAlloc: {
            if (expr->arena->kind != ExprKind::Identifier) unsupported("arena target must be an identifier", expr->line);
            const auto arena = arenaAddressFor(expr->arena->strValue, expr->line);
            return emitDynamicArrayLiteral(expr->value.get(), arena);
        }

        case ExprKind::Call: {
            if (expr->callee->kind == ExprKind::FieldAccess) {
                const auto* field = expr->callee.get();
                if (field->target->kind == ExprKind::Identifier && field->target->strValue == "Arena" && field->field == "create") {
                    return emitArenaCreate(expr);
                }
                if (field->target->kind == ExprKind::Identifier &&
                    (field->target->strValue == "Stdin" || field->target->strValue == "Clock" ||
                     field->target->strValue == "Cpu" || field->target->strValue == "Thread" ||
                     field->target->strValue == "Atomic" || field->target->strValue == "Web" ||
                     field->target->strValue == "Net" || field->target->strValue == "Poller" ||
                     field->target->strValue == "Mutex" || field->target->strValue == "RwLock" ||
                     field->target->strValue == "Condvar" || field->target->strValue == "Semaphore" ||
                     field->target->strValue == "Process" || field->target->strValue == "Http" ||
                     field->target->strValue == "Json" || field->target->strValue == "Tensor" ||
                     field->target->strValue == "Grad" || field->target->strValue == "Accel" ||
                     field->target->strValue == "Mobile" || field->target->strValue == "Game" ||
                     field->target->strValue == "Graphics" || field->target->strValue == "Args" ||
                     field->target->strValue == "Env" || field->target->strValue == "FS" ||
                     field->target->strValue == "Path" || field->target->strValue == "Regex" ||
                     field->target->strValue == "Shell" || field->target->strValue == "String")) {
                    return emitBuiltinCall(expr);
                }
                const auto* targetTypeRaw = field->target->checkedType.get();
                if (!targetTypeRaw) unsupported("built-in method target has no type", expr->line);
                auto targetType = cloneType(targetTypeRaw);
                if (targetType->isReference) targetType->isReference = false;
                // Mirrors emitDynamicArrayPush's proven convention: emitExpr() on a
                // reference-typed target yields the pointer to the real aggregate
                // storage (one dereference), not the aggregate value itself, so an
                // explicit second load is required to read it. len()/isEmpty()
                // previously fed that pointer straight into extractvalue, which is
                // only correct for non-reference (owned) targets.
                const auto loadAggregate = [&](const std::string& llvmAggType) {
                    const std::string addr = targetTypeRaw->isReference ? emitExpr(field->target.get())
                                                                          : emitLValueAddress(field->target.get());
                    const auto value = newTemp("aggregate.load");
                    body.push_back("  " + value + " = load " + llvmAggType + ", ptr " + addr);
                    return value;
                };
                if (targetType->isRawPointer && (field->field == "add" || field->field == "sub" || field->field == "offset")) {
                    if (expr->args.size() != 1) unsupported("raw pointer arithmetic takes one offset", expr->line);
                    const auto ptr = emitExpr(field->target.get());
                    auto pointee = cloneType(targetType.get());
                    pointee->isRawPointer = false;
                    pointee->isMutable = false;
                    auto off = emitExpr(expr->args[0].get());
                    const auto offType = exprLLVMType(expr->args[0].get());
                    if (offType != "i64") {
                        auto normalized = newTemp("ptr.offset");
                        const bool signedOff = expr->args[0]->checkedType && isSignedType(expr->args[0]->checkedType.get());
                        body.push_back("  " + normalized + " = " + (signedOff ? "sext" : "zext") + " " + offType + " " + off + " to i64");
                        off = normalized;
                    }
                    std::string index = off;
                    if (field->field == "sub" || field->field == "offset") {
                        if (field->field == "sub") {
                            const auto neg = newTemp("ptr.neg");
                            body.push_back("  " + neg + " = sub i64 0, " + off);
                            index = neg;
                        }
                    }
                    const auto r = newTemp("ptr.arith");
                    body.push_back("  " + r + " = getelementptr " + llvmType(pointee.get()) + ", ptr " + ptr + ", i64 " + index);
                    return r;
                }
                if (targetType->name == "Tensor") {
                    const auto base = emitExpr(field->target.get());
                    const auto member = field->field;
                    auto asU64 = [&](std::size_t i) { auto v=emitExpr(expr->args[i].get()); auto t=exprLLVMType(expr->args[i].get()); if(t!="i64"){auto w=newTemp("tensor.u64");body.push_back("  "+w+" = zext "+t+" "+v+" to i64");v=w;} return v; };
                    auto asF64 = [&](std::size_t i) { auto v=emitExpr(expr->args[i].get()); auto t=exprLLVMType(expr->args[i].get()); if(t=="float"){auto w=newTemp("tensor.f64");body.push_back("  "+w+" = fpext float "+v+" to double");v=w;} return v; };
                    auto tensorArg=[&](std::size_t i){return emitExpr(expr->args[i].get());};
                    if(member=="clone"||member=="contiguous"){auto r=newTemp("tensor.clone");body.push_back("  "+r+" = call ptr @"+std::string(member=="clone"?"__lanner_tensor_clone":"__lanner_tensor_contiguous")+"(ptr "+base+")");return r;}
                    if(member=="free"){body.push_back("  call void @__lanner_tensor_free(ptr "+base+")");return {};}
                    if(member=="rank"||member=="len"){auto r=newTemp("tensor.query");body.push_back("  "+r+" = call i64 @"+std::string(member=="rank"?"__lanner_tensor_rank":"__lanner_tensor_len")+"(ptr "+base+")");return r;}
                    if(member=="dim"||member=="stride"){auto r=newTemp("tensor.query");body.push_back("  "+r+" = call i64 @"+std::string(member=="dim"?"__lanner_tensor_dim":"__lanner_tensor_stride")+"(ptr "+base+", i64 "+asU64(0)+")");return r;}
                    if(member=="dtype"){auto r=newTemp("tensor.dtype");body.push_back("  "+r+" = call i32 @__lanner_tensor_dtype(ptr "+base+")");return r;}
                    if(member=="isContiguous"){auto r=newTemp("tensor.contig");body.push_back("  "+r+" = call i32 @__lanner_tensor_is_contiguous(ptr "+base+")");auto b=newTemp("tensor.contig.bool");body.push_back("  "+b+" = trunc i32 "+r+" to i1");return b;}
                    if(member=="dataF32"||member=="dataF64"){auto r=newTemp("tensor.data");body.push_back("  "+r+" = call ptr @"+std::string(member=="dataF32"?"__lanner_tensor_data_f32":"__lanner_tensor_data_f64")+"(ptr "+base+")");return r;}
                    if(member=="get1"||member=="get2"||member=="get3"){int n=member.back()-'0';std::string a="ptr "+base;for(int i=0;i<n;++i)a+=", i64 "+asU64(i);auto r=newTemp("tensor.get");body.push_back("  "+r+" = call double @__lanner_tensor_get"+std::to_string(n)+"("+a+")");return r;}
                    if(member=="set1"||member=="set2"||member=="set3"){int n=member.back()-'0';std::string a="ptr "+base;for(int i=0;i<n;++i)a+=", i64 "+asU64(i);a+=", double "+asF64(n);body.push_back("  call void @__lanner_tensor_set"+std::to_string(n)+"("+a+")");return {};}
                    if(member=="add"||member=="sub"||member=="mul"||member=="div"||member=="matmul"){const char*f=member=="add"?"__lanner_tensor_add":member=="sub"?"__lanner_tensor_sub":member=="mul"?"__lanner_tensor_mul":member=="div"?"__lanner_tensor_div":"__lanner_tensor_matmul";auto r=newTemp("tensor.op");body.push_back("  "+r+" = call ptr @"+f+"(ptr "+base+", ptr "+tensorArg(0)+")");return r;}
                    if(member=="scale"){auto r=newTemp("tensor.scale");body.push_back("  "+r+" = call ptr @__lanner_tensor_scale(ptr "+base+", double "+asF64(0)+")");return r;}
                    if(member=="relu"||member=="sigmoid"||member=="tanh"){const char*f=member=="relu"?"__lanner_tensor_relu":member=="sigmoid"?"__lanner_tensor_sigmoid":"__lanner_tensor_tanh";auto r=newTemp("tensor.act");body.push_back("  "+r+" = call ptr @"+f+"(ptr "+base+")");return r;}
                    if(member=="softmax"){auto r=newTemp("tensor.softmax");body.push_back("  "+r+" = call ptr @__lanner_tensor_softmax(ptr "+base+", i64 "+asU64(0)+")");return r;}
                    if(member=="sum"||member=="mean"||member=="l2Norm"){const char*f=member=="sum"?"__lanner_tensor_sum":member=="mean"?"__lanner_tensor_mean":"__lanner_tensor_l2norm";auto r=newTemp("tensor.reduce");body.push_back("  "+r+" = call double @"+f+"(ptr "+base+")");return r;}
                    if(member=="dot"){auto r=newTemp("tensor.dot");body.push_back("  "+r+" = call double @__lanner_tensor_dot(ptr "+base+", ptr "+tensorArg(0)+")");return r;}
                    if(member=="argmax"){auto r=newTemp("tensor.argmax");body.push_back("  "+r+" = call i64 @__lanner_tensor_argmax(ptr "+base+", i64 "+asU64(0)+")");return r;}
                    if(member=="reshape2"||member=="reshape3"||member=="reshape4"){int n=member.back()-'0';std::string a="ptr "+base;for(int i=0;i<n;++i)a+=", i64 "+asU64(i);auto r=newTemp("tensor.reshape");body.push_back("  "+r+" = call ptr @__lanner_tensor_reshape"+std::to_string(n)+"("+a+")");return r;}
                    if(member=="transpose2"){auto r=newTemp("tensor.transpose");body.push_back("  "+r+" = call ptr @__lanner_tensor_transpose2(ptr "+base+")");return r;}
                    if(member=="slice"){auto r=newTemp("tensor.slice");body.push_back("  "+r+" = call ptr @__lanner_tensor_slice(ptr "+base+", i64 "+asU64(0)+", i64 "+asU64(1)+", i64 "+asU64(2)+", i64 "+asU64(3)+")");return r;}
                    if(member=="fill"){body.push_back("  call void @__lanner_tensor_fill(ptr "+base+", double "+asF64(0)+")");return {};}
                    if(member=="conv2d"){auto r=newTemp("tensor.conv2d");body.push_back("  "+r+" = call ptr @__lanner_tensor_conv2d(ptr "+base+", ptr "+tensorArg(0)+", i64 "+asU64(1)+", i64 "+asU64(2)+")");return r;}
                }
                if (targetType->name == "GradTape") {
                    if (field->field == "free") { auto h=emitExpr(field->target.get()); body.push_back("  call void @__lanner_grad_free(ptr "+h+")"); return {}; }
                }
                if (targetType->name == "Buffer") {
                    const auto b = emitExpr(field->target.get());
                    if (field->field == "len") { auto r=newTemp("buffer.len"); body.push_back("  "+r+" = call i64 @__lanner_buffer_len(ptr "+b+")"); if(pointerBits==64)return r; auto q=newTemp("buffer.len.narrow"); body.push_back("  "+q+" = trunc i64 "+r+" to i32"); return q; }
                    if (field->field == "data") { auto r=newTemp("buffer.data"); body.push_back("  "+r+" = call ptr @__lanner_buffer_data(ptr "+b+")"); return r; }
                    if (field->field == "cstr") { auto r=newTemp("buffer.cstr"); body.push_back("  "+r+" = call ptr @__lanner_buffer_cstr(ptr "+b+")"); return r; }
                    if (field->field == "free") { body.push_back("  call void @__lanner_buffer_free(ptr "+b+")"); return {}; }
                    if (field->field == "appendString") { auto t=emitExpr(expr->args[0].get()); auto r=newTemp("buffer.append"); body.push_back("  "+r+" = call i32 @__lanner_buffer_append_string(ptr "+b+", ptr "+t+")"); auto q=newTemp("buffer.append.bool"); body.push_back("  "+q+" = trunc i32 "+r+" to i1"); return q; }
                    if (field->field == "appendBuffer") { auto o=emitExpr(expr->args[0].get()); auto r=newTemp("buffer.append"); body.push_back("  "+r+" = call i32 @__lanner_buffer_append_buffer(ptr "+b+", ptr "+o+")"); auto q=newTemp("buffer.append.bool"); body.push_back("  "+q+" = trunc i32 "+r+" to i1"); return q; }
                }
                if (targetType->name == "string" && !targetType->isArray) {
                    const auto text = emitExpr(field->target.get());
                    if (field->field == "startsWith") { auto pref=emitExpr(expr->args[0].get()); auto r=newTemp("str.lanarts"); body.push_back("  "+r+" = call i32 @__lanner_string_startsWith(ptr "+text+", ptr "+pref+")"); auto b=newTemp("str.lanarts.bool"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b; }
                    if (field->field == "contains" || field->field == "endsWith" || field->field == "equalsIgnoreCase") { auto other=emitExpr(expr->args[0].get()); auto r=newTemp("str.bool"); body.push_back("  "+r+" = call i32 @__lanner_string_"+field->field+"(ptr "+text+", ptr "+other+")"); auto b=newTemp("str.bool.cast"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b; }
                    if (field->field == "equals") { auto other=emitExpr(expr->args[0].get()); auto r=newTemp("str.eq"); body.push_back("  "+r+" = call i32 @__lanner_string_equals(ptr "+text+", ptr "+other+")"); auto b=newTemp("str.eq.bool"); body.push_back("  "+b+" = trunc i32 "+r+" to i1"); return b; }
                    if (field->field == "find") { auto other=emitExpr(expr->args[0].get()); auto r=newTemp("str.find"); body.push_back("  "+r+" = call i64 @__lanner_string_find(ptr "+text+", ptr "+other+")"); return r; }
                    if (field->field == "parseU64At") { auto pos=emitExpr(expr->args[0].get()); auto r=newTemp("str.parse.u64"); body.push_back("  "+r+" = call i64 @__lanner_string_parse_u64_at(ptr "+text+", i64 "+pos+")"); return r; }
                }
                if (targetType->name == "Atomic" && targetType->generics.size() == 1) {
                    const std::string elem = llvmType(targetType->generics[0].get());
                    const std::string addr = targetTypeRaw->isReference ? emitExpr(field->target.get()) : emitLValueAddress(field->target.get());
                    const int bits = (targetType->generics[0]->name == "usize" || targetType->generics[0]->name == "isize") ? pointerBits : intBits(targetType->generics[0]->name);
                    const std::string align = std::to_string(std::max(1, bits / 8));
                    const auto orderAt = [&](std::size_t index) { return index < expr->args.size() ? expr->args[index]->strValue : std::string("seq_cst"); };
                    const auto llvmOrder = [](const std::string& order) { return order == "relaxed" ? std::string("monotonic") : order; };
                    if (field->field == "load") {
                        const auto order = llvmOrder(orderAt(0));
                        auto r=newTemp("atomic.load"); body.push_back("  "+r+" = load atomic "+elem+", ptr "+addr+" "+order+", align "+align); return r;
                    }
                    if (field->field == "store") {
                        auto v=emitExpr(expr->args[0].get()); const auto order = llvmOrder(orderAt(1));
                        body.push_back("  store atomic "+elem+" "+v+", ptr "+addr+" "+order+", align "+align); return "";
                    }
                    if (field->field == "fetchAdd" || field->field == "fetchSub") {
                        auto v=emitExpr(expr->args[0].get()); const auto order = llvmOrder(orderAt(1)); auto r=newTemp("atomic.fetch"); body.push_back("  "+r+" = atomicrmw "+(field->field=="fetchAdd"?"add":"sub")+" ptr "+addr+", "+elem+" "+v+" "+order+", align "+align); return r;
                    }
                    if (field->field == "compareExchange") {
                        auto e=emitExpr(expr->args[0].get()); auto d=emitExpr(expr->args[1].get()); const auto success = orderAt(2);
                        const auto failure = success == "release" ? "monotonic" : success == "acq_rel" ? "acquire" : llvmOrder(success);
                        auto pair=newTemp("atomic.cmpxchg"); body.push_back("  "+pair+" = cmpxchg ptr "+addr+", "+elem+" "+e+", "+elem+" "+d+" "+llvmOrder(success)+" "+failure+", align "+align);
                        auto old=newTemp("atomic.old"); body.push_back("  "+old+" = extractvalue { "+elem+", i1 } "+pair+", 0"); auto ok=newTemp("atomic.ok"); body.push_back("  "+ok+" = extractvalue { "+elem+", i1 } "+pair+", 1"); return ok;
                    }
                }
                if (targetType->name == "Thread" && field->field == "join") { auto h=emitExpr(field->target.get()); body.push_back("  call void @__lanner_thread_join(ptr "+h+")"); return ""; }
                if (targetType->name == "Thread" && field->field == "detach") { auto h=emitExpr(field->target.get()); body.push_back("  call void @__lanner_thread_detach(ptr "+h+")"); return ""; }
                if ((targetType->name == "v128" || targetType->name == "v256" || targetType->name == "v512") &&
                    (field->field == "add" || field->field == "and" || field->field == "or" || field->field == "xor")) {
                    const std::string ty = targetType->name=="v128"?"<2 x i64>":targetType->name=="v256"?"<4 x i64>":"<8 x i64>";
                    auto lhs=emitExpr(field->target.get()); auto rhs=emitExpr(expr->args[0].get()); auto r=newTemp("simd");
                    const char* op = field->field=="add"?"add":field->field=="and"?"and":field->field=="or"?"or":"xor";
                    body.push_back("  "+r+" = "+op+" "+ty+" "+lhs+", "+rhs); return r;
                }
                if ((targetType->name == "v128" || targetType->name == "v256" || targetType->name == "v512") && field->field == "extractU64") {
                    const std::string ty = targetType->name=="v128"?"<2 x i64>":targetType->name=="v256"?"<4 x i64>":"<8 x i64>";
                    auto vec=emitExpr(field->target.get()); auto lane=emitExpr(expr->args[0].get()); auto r=newTemp("simd.extract"); body.push_back("  "+r+" = extractelement "+ty+" "+vec+", i32 "+lane); return r;
                }
                if (field->field == "len") {
                    if (targetType->name == "string" && !targetType->isArray) {
                        const auto addrOrValue = targetTypeRaw->isReference ? emitExpr(field->target.get()) : emitExpr(field->target.get());
                        const auto text = targetTypeRaw->isReference ? [&]() {
                            const auto loaded = newTemp("string.ref.load");
                            body.push_back("  " + loaded + " = load ptr, ptr " + addrOrValue);
                            return loaded;
                        }() : addrOrValue;
                        const auto value = newTemp("string.len.value");
                        body.push_back("  " + value + " = call i64 @strlen(ptr " + text + ")");
                        return value;
                    }
                    if (targetType->isArray && targetType->fixedArraySize) return std::to_string(*targetType->fixedArraySize);
                    if (targetType->isArray) {
                        const auto target = loadAggregate("%LannerDynArray");
                        const auto value = newTemp("array.len.value");
                        body.push_back("  " + value + " = extractvalue %LannerDynArray " + target + ", 1");
                        return value;
                    }
                    if (targetType->name == "View" || targetType->name == "EditView") {
                        const auto target = loadAggregate("{ ptr, i64 }");
                        const auto value = newTemp("view.len.value");
                        body.push_back("  " + value + " = extractvalue { ptr, i64 } " + target + ", 1");
                        return value;
                    }
                }
                if (field->field == "isEmpty") {
                    std::string length;
                    if (targetType->isArray && targetType->fixedArraySize) length = std::to_string(*targetType->fixedArraySize);
                    else if (targetType->isArray) {
                        const auto target = loadAggregate("%LannerDynArray");
                        length = newTemp("array.empty.len");
                        body.push_back("  " + length + " = extractvalue %LannerDynArray " + target + ", 1");
                    } else if (targetType->name == "View" || targetType->name == "EditView") {
                        const auto target = loadAggregate("{ ptr, i64 }");
                        length = newTemp("view.empty.len");
                        body.push_back("  " + length + " = extractvalue { ptr, i64 } " + target + ", 1");
                    } else unsupported("isEmpty() is not supported on this type", expr->line);
                    const auto result = newTemp("empty");
                    body.push_back("  " + result + " = icmp eq i64 " + length + ", 0");
                    return result;
                }
                if (field->field == "push") {
                    if (expr->args.size() != 1) unsupported("push() takes one argument", expr->line);
                    return emitDynamicArrayPush(field->target.get(), expr->args[0].get(), expr->line);
                }
                unsupported("unsupported built-in method '" + field->field + "'", expr->line);
            }

            if (expr->callee->kind != ExprKind::Identifier) unsupported("only direct function calls are lowered", expr->line);
            const auto& directName = expr->callee->strValue;
            if (directName == "readFile" || directName == "writeStdout" ||
                directName == "writeRaw" || directName == "writeIntRaw" ||
                directName == "writeByteRaw" || directName == "print" ||
                directName == "printInt" || directName == "stringLen" ||
                directName == "getEnv" || directName == "alloc" || directName == "realloc" ||
                directName == "dealloc" || directName == "allocAligned" ||
                directName == "deallocAligned" || directName == "volatileLoad" ||
                directName == "volatileStore" || directName == "unalignedLoad" ||
                directName == "unalignedStore" || directName == "memset" ||
                directName == "memcpy" || directName == "memmove" || directName == "memcmp" ||
                directName == "stackAlloc" || directName == "assume" || directName == "trap" ||
                directName == "sizeOf" || directName == "alignOf" || directName == "offsetOf" || directName == "ptrDiff" ||
                directName == "atomicFence" || directName == "compilerFence" ||
                directName == "unreachable" || directName == "asm" || directName == "asmI64" || directName == "asmI32" || directName == "asmPtr") {
                return emitBuiltinCall(expr);
            }
            if (auto* localFn = lookupLocal(expr->callee->strValue); localFn && localFn->type && localFn->type->name == "fn") {
                const auto* fnType = localFn->type;
                const auto ret = llvmType(fnType->generics.back().get());
                const auto calleeValue = emitExpr(expr->callee.get());
                std::string call = "call " + ret + " " + calleeValue + "(";
                for (std::size_t i = 0; i < expr->args.size(); ++i) {
                    if (i) call += ", ";
                    call += llvmType(fnType->generics[i].get()) + " " + emitExpr(expr->args[i].get());
                }
                call += ")";
                if (ret == "void") { body.push_back("  " + call); return {}; }
                const auto result = newTemp("fnptr.call");
                body.push_back("  " + result + " = " + call);
                return result;
            }
            auto it = functions.find(expr->callee->strValue);
            if (it == functions.end()) unsupported("unknown function", expr->line);
            const auto* fn = it->second;
            const auto ret = llvmType(fn->returnType.get());
            std::string call = "call " + ret + " @" + fn->name + "(";
            for (std::size_t i = 0; i < expr->args.size(); ++i) {
                if (i) call += ", ";
                const auto argValue = emitExpr(expr->args[i].get());
                call += llvmType(fn->params[i].type.get()) + " " + argValue;
                markMovedArgument(expr->args[i].get(), fn->params[i].type.get());
            }
            call += ")";
            if (ret == "void") {
                body.push_back("  " + call);
                return "";
            }
            const auto result = newTemp("call");
            body.push_back("  " + result + " = " + call);
            return result;
        }

        case ExprKind::Cast: {
            const auto value = emitExpr(expr->value.get());
            const auto from = exprLLVMType(expr->value.get());
            const auto to = llvmType(expr->castType.get());
            if (from == to) return value;
            const auto result = newTemp("cast");
            if (from == "ptr" && isIntegerLLVM(to)) {
                body.push_back("  " + result + " = ptrtoint ptr " + value + " to " + to);
                return result;
            }
            if (isIntegerLLVM(from) && to == "ptr") {
                body.push_back("  " + result + " = inttoptr " + from + " " + value + " to ptr");
                return result;
            }
            if (from == "ptr" && to == "ptr") return value;
            if (isIntegerLLVM(from) && isIntegerLLVM(to)) {
                const int fromBits = std::stoi(from.substr(1));
                const int toBits = std::stoi(to.substr(1));
                if (toBits < fromBits) {
                    body.push_back("  " + result + " = trunc " + from + " " + value + " to " + to);
                } else if (toBits > fromBits) {
                    const bool signedSource = expr->value->checkedType && isSignedType(expr->value->checkedType.get());
                    body.push_back("  " + result + " = " + (signedSource ? "sext" : "zext") + " " + from + " " + value + " to " + to);
                } else {
                    return value;
                }
                return result;
            }
            if ((from == "float" || from == "double") && (to == "float" || to == "double")) {
                body.push_back("  " + result + " = " + (to == "double" ? "fpext" : "fptrunc") + " " + from + " " + value + " to " + to);
                return result;
            }
            if (isIntegerLLVM(from) && (to == "float" || to == "double")) {
                const bool signedSource = expr->value->checkedType && isSignedType(expr->value->checkedType.get());
                body.push_back("  " + result + " = " + (signedSource ? "sitofp" : "uitofp") + " " + from + " " + value + " to " + to);
                return result;
            }
            if ((from == "float" || from == "double") && isIntegerLLVM(to)) {
                const bool signedDest = expr->castType && isSignedType(expr->castType.get());
                body.push_back("  " + result + " = " + (signedDest ? "fptosi" : "fptoui") + " " + from + " " + value + " to " + to);
                return result;
            }
            if (from == "i1" && isIntegerLLVM(to)) {
                body.push_back("  " + result + " = zext i1 " + value + " to " + to);
                return result;
            }
            if (isIntegerLLVM(from) && to == "i1") {
                body.push_back("  " + result + " = icmp ne " + from + " " + value + ", 0");
                return result;
            }
            unsupported("unsupported cast", expr->line);
        }

        case ExprKind::ArrayLit: {
            if (!expr->checkedType || !expr->checkedType->isArray) {
                unsupported("array literal has no array type", expr->line);
            }
            if (!expr->checkedType->fixedArraySize) {
                return emitDynamicArrayLiteral(expr, "");
            }
            const auto arrayType = llvmType(expr->checkedType.get());
            const auto addr = "%arraylit.addr" + std::to_string(labelCounter++);
            body.push_back("  " + addr + " = alloca " + arrayType);
            auto elementType = cloneType(expr->checkedType.get());
            elementType->isArray = false;
            elementType->fixedArraySize.reset();
            elementType->origin = {};
            for (std::size_t i = 0; i < expr->args.size(); ++i) {
                const auto val = emitExpr(expr->args[i].get());
                const auto elem = newTemp("array.elem");
                body.push_back("  " + elem + " = getelementptr inbounds " + arrayType + ", ptr " + addr + ", i64 0, i64 " + std::to_string(i));
                body.push_back("  store " + llvmType(elementType.get()) + " " + val + ", ptr " + elem);
                markMovedArgument(expr->args[i].get(), nullptr);
            }
            const auto result = newTemp("array.load");
            body.push_back("  " + result + " = load " + arrayType + ", ptr " + addr);
            return result;
        }

        case ExprKind::StructLit: {
            if (!expr->checkedType || !structs.count(expr->checkedType->name)) unsupported("struct literal has no known struct type", expr->line);
            const auto structType = llvmType(expr->checkedType.get());
            const auto* st = structs.at(expr->checkedType->name);
            const auto addr = "%structlit.addr" + std::to_string(labelCounter++);
            body.push_back("  " + addr + " = alloca " + structType);
            for (const auto& supplied : expr->fields) {
                std::size_t index = 0;
                const StructField* field = nullptr;
                for (std::size_t i = 0; i < st->fields.size(); ++i) {
                    if (st->fields[i].name == supplied.first) { index = i; field = &st->fields[i]; break; }
                }
                if (!field) unsupported("unknown struct field '" + supplied.first + "'", expr->line);
                const auto value = emitExpr(supplied.second.get());
                const auto fieldAddr = newTemp("struct.field.addr");
                body.push_back("  " + fieldAddr + " = getelementptr inbounds " + structType + ", ptr " + addr + ", i32 0, i32 " + std::to_string(index));
                body.push_back("  store " + llvmType(field->type.get()) + " " + value + ", ptr " + fieldAddr);
                markMovedArgument(supplied.second.get(), nullptr);
            }
            const auto result = newTemp("struct.load");
            body.push_back("  " + result + " = load " + structType + ", ptr " + addr);
            return result;
        }

        case ExprKind::Index: {
            // Strings are zero-terminated byte buffers, not Lanner array aggregates.
            // Lower string indexing directly to a checked i8 load instead of asking
            // emitLValueAddress() for an address in a non-lvalue aggregate.
            if (expr->target->checkedType && expr->target->checkedType->name == "string" &&
                !expr->target->checkedType->isReference && expr->args.size() == 1) {
                const auto text = emitExpr(expr->target.get());
                const auto indexValue = emitExpr(expr->args[0].get());
                const auto indexI64 = normalizeIndexToI64(expr->args[0].get(), indexValue);
                const auto length = newTemp("string.length");
                body.push_back("  " + length + " = call i64 @strlen(ptr " + text + ")");
                emitDynamicBoundsCheck(indexI64, length);
                const auto address = newTemp("string.elem.addr");
                body.push_back("  " + address + " = getelementptr inbounds i8, ptr " + text + ", i64 " + indexI64);
                const auto value = newTemp("string.elem");
                body.push_back("  " + value + " = load i8, ptr " + address);
                return value;
            }
            const auto address = emitLValueAddress(expr);
            const auto value = newTemp("elem");
            body.push_back("  " + value + " = load " + exprLLVMType(expr) + ", ptr " + address);
            return value;
        }

        case ExprKind::Slice: {
            if (expr->args.size() != 2) unsupported("slice needs lower and upper bounds", expr->line);
            const auto* checked = expr->checkedType.get();
            if (!checked || (checked->name != "View" && checked->name != "EditView") || checked->generics.size() != 1)
                unsupported("slice does not lower to a View", expr->line);
            const auto elemType = llvmType(checked->generics[0].get());
            const auto* sourceType = expr->target->checkedType.get();
            if (!sourceType) unsupported("slice source has no type", expr->line);
            auto normalized = cloneType(sourceType);
            if (normalized->isReference) normalized->isReference = false;

            std::string base;
            std::string limit;
            if (normalized->isArray && normalized->fixedArraySize) {
                const auto* local = expr->target->kind == ExprKind::Identifier ? lookupLocal(expr->target->strValue) : nullptr;
                if (sourceType->isReference) base = emitExpr(expr->target.get());
                else if (local) base = local->addr;
                else unsupported("fixed-array slices currently require a local array", expr->line);
                limit = std::to_string(*normalized->fixedArraySize);
            } else if (normalized->isArray) {
                const auto arrayValue = sourceType->isReference ? [&]() {
                    const auto p = emitExpr(expr->target.get());
                    const auto loaded = newTemp("slice.array.ref");
                    body.push_back("  " + loaded + " = load %LannerDynArray, ptr " + p);
                    return loaded;
                }() : emitExpr(expr->target.get());
                const auto p = newTemp("slice.array.ptr");
                const auto l = newTemp("slice.array.len");
                body.push_back("  " + p + " = extractvalue %LannerDynArray " + arrayValue + ", 0");
                body.push_back("  " + l + " = extractvalue %LannerDynArray " + arrayValue + ", 1");
                base = p;
                limit = l;
            } else if (normalized->name == "View" || normalized->name == "EditView") {
                const auto viewValue = sourceType->isReference ? [&]() {
                    const auto p = emitExpr(expr->target.get());
                    const auto loaded = newTemp("slice.view.ref");
                    body.push_back("  " + loaded + " = load { ptr, i64 }, ptr " + p);
                    return loaded;
                }() : emitExpr(expr->target.get());
                const auto p = newTemp("slice.view.ptr");
                const auto l = newTemp("slice.view.len");
                body.push_back("  " + p + " = extractvalue { ptr, i64 } " + viewValue + ", 0");
                body.push_back("  " + l + " = extractvalue { ptr, i64 } " + viewValue + ", 1");
                base = p;
                limit = l;
            } else unsupported("unsupported slice source", expr->line);

            const auto lowRaw = expr->args[0] ? emitExpr(expr->args[0].get()) : "0";
            const auto highRaw = expr->args[1] ? emitExpr(expr->args[1].get()) : limit;
            const auto low = expr->args[0] ? normalizeIndexToI64(expr->args[0].get(), lowRaw) : "0";
            const auto high = expr->args[1] ? normalizeIndexToI64(expr->args[1].get(), highRaw) : limit;
            const auto ordered = newTemp("slice.ordered");
            const auto inside = newTemp("slice.inside");
            const auto valid = newTemp("slice.valid");
            body.push_back("  " + ordered + " = icmp ule i64 " + low + ", " + high);
            body.push_back("  " + inside + " = icmp ule i64 " + high + ", " + limit);
            body.push_back("  " + valid + " = and i1 " + ordered + ", " + inside);
            const auto ok = newLabel("slice.ok");
            const auto bad = newLabel("slice.trap");
            body.push_back("  br i1 " + valid + ", label %" + ok + ", label %" + bad);
            body.push_back(bad + ":");
            body.push_back("  call void @llvm.trap()");
            body.push_back("  unreachable");
            body.push_back(ok + ":");

            std::string pointer;
            if (normalized->isArray && normalized->fixedArraySize && !sourceType->isReference) {
                const auto arrayType = llvmType(normalized.get());
                pointer = newTemp("slice.ptr");
                body.push_back("  " + pointer + " = getelementptr inbounds " + arrayType + ", ptr " + base + ", i64 0, i64 " + low);
            } else {
                pointer = newTemp("slice.ptr");
                body.push_back("  " + pointer + " = getelementptr inbounds " + elemType + ", ptr " + base + ", i64 " + low);
            }
            const auto length = newTemp("slice.len");
            body.push_back("  " + length + " = sub i64 " + high + ", " + low);
            const auto v0 = newTemp("view");
            const auto v1 = newTemp("view");
            body.push_back("  " + v0 + " = insertvalue { ptr, i64 } zeroinitializer, ptr " + pointer + ", 0");
            body.push_back("  " + v1 + " = insertvalue { ptr, i64 } " + v0 + ", i64 " + length + ", 1");
            return v1;
        }

        default:
            unsupported("expression kind is not yet lowered", expr->line);
    }
}

void LLVMCodeGenerator::emitStmt(const Stmt* stmt) {
    if (blockTerminated) unsupported("statement follows terminated block", stmt ? stmt->line : 0);

    if (stmt && stmt->kind == StmtKind::ComptimeDecl) {
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

    switch (stmt->kind) {
        case StmtKind::Assign:
        case StmtKind::ConstAssign: {
            if (stmt->expr) {
                const auto address = emitLValueAddress(stmt->expr.get());
                const auto value = emitExpr(stmt->assignValue.get());
                LocalBinding destination{stmt->expr->checkedType.get(), address, false, false};
                cleanupBinding("assignment", destination);
                body.push_back("  store " + exprLLVMType(stmt->assignValue.get()) + " " + value + ", ptr " + address);
                markMovedArgument(stmt->assignValue.get(), nullptr);
                break;
            }

            LocalBinding* existing = lookupLocal(stmt->assignTarget);
            std::unique_ptr<TypeNode> referencePointee;
            TypeNode* type = existing ? existing->type : stmt->declaredType.get();
            if (existing && existing->type && existing->type->isReference && !stmt->declaredType) {
                referencePointee = cloneType(existing->type);
                referencePointee->isReference = false;
                referencePointee->isMutable = false;
                referencePointee->origin = {};
                referencePointee->nestedOrigins.clear();
                type = referencePointee.get();
            }
            if (!type) {
                auto rhsType = stmt->assignValue->checkedType ? cloneType(stmt->assignValue->checkedType.get()) : nullptr;
                if (!rhsType) unsupported("backend needs a checked local type", stmt->line);
                ownedLocalTypes.push_back(std::move(rhsType));
                type = ownedLocalTypes.back().get();
            }

            std::string address;
            if (!existing) {
                auto staticIt = statics.find(stmt->assignTarget);
                if (staticIt != statics.end()) {
                    address = "@" + stmt->assignTarget;
                    type = staticIt->second->type.get();
                } else {
                    if (localScopes.empty()) pushScope();
                    address = "%" + stmt->assignTarget + ".addr" + std::to_string(labelCounter++);
                    localScopes.back()[stmt->assignTarget] = LocalBinding{type, address, false, false};
                    scopeOrder.back().push_back(stmt->assignTarget);
                    body.push_back("  " + address + " = alloca " + llvmType(type));
                }
            } else {
                if (referencePointee) {
                    // The bare-reference assignment has no Expr target in the AST;
                    // the pointee address is loaded from the reference binding below.
                    address.clear();
                } else {
                    address = existing->addr;
                    type = existing->type;
                    // Assignment into an existing owning slot first destroys the old value.
                    // cleanupBinding is a no-op for copyable scalars/views.
                    if (!existing->moved) cleanupBinding(stmt->assignTarget, *existing);
                }
            }

            if (referencePointee) {
                const auto refObject = newTemp("ref.assign");
                body.push_back("  " + refObject + " = load ptr, ptr " + existing->addr);
                address = refObject;
            }
            const auto value = emitExpr(stmt->assignValue.get());
            if (type->name == "Atomic" && type->generics.size() == 1) {
                const int bits = (type->generics[0]->name == "usize" || type->generics[0]->name == "isize") ? pointerBits : intBits(type->generics[0]->name);
                body.push_back("  store atomic " + llvmType(type) + " " + value + ", ptr " + address + " seq_cst, align " + std::to_string(std::max(1, bits / 8)));
            } else {
                body.push_back("  store " + llvmType(type) + " " + value + ", ptr " + address);
            }
            if (stmt->assignValue->kind == ExprKind::Identifier && stmt->assignValue->checkedType &&
                !TypeChecker::isCopyType(stmt->assignValue->checkedType.get())) {
                if (auto* source = lookupLocal(stmt->assignValue->strValue); source && source != existing) source->moved = true;
            }
            if (existing) existing->moved = false, existing->cleaned = false;
            break;
        }

        case StmtKind::ExprStmt:
            (void)emitExpr(stmt->expr.get());
            break;

        case StmtKind::UnsafeBlock:
            // Unsafe is a semantic boundary only. It has no runtime representation.
            (void)emitBlock(stmt->body);
            break;

        case StmtKind::Return:
            if (!stmt->expr) {
                cleanupAllScopes();
                body.push_back("  ret void");
            } else {
                const auto value = emitExpr(stmt->expr.get());
                if (stmt->expr->kind == ExprKind::Identifier && stmt->expr->checkedType &&
                    !TypeChecker::isCopyType(stmt->expr->checkedType.get())) {
                    if (auto* source = lookupLocal(stmt->expr->strValue)) source->moved = true;
                }
                cleanupAllScopes();
                body.push_back("  ret " + llvmType(currentFunction->returnType.get()) + " " + value);
            }
            blockTerminated = true;
            break;

        case StmtKind::Guard: {
            const auto thenLabel = newLabel("guard.then");
            const auto mergeLabel = newLabel("guard.end");

            std::string condition;
            std::string resultValue;
            const Expr* resultOperand = nullptr;
            bool isResultMatch = false;
            if (stmt->guardCondition && stmt->guardCondition->kind == ExprKind::IsMatch) {
                resultOperand = stmt->guardCondition->left.get();
                if (!resultOperand || !resultOperand->checkedType ||
                    resultOperand->checkedType->name != "Result" || resultOperand->checkedType->generics.size() != 2) {
                    unsupported("guard Result match has no complete Result type", stmt->line);
                }
                resultValue = emitExpr(resultOperand);
                const auto tag = newTemp("guard.tag");
                body.push_back("  " + tag + " = extractvalue " + llvmType(resultOperand->checkedType.get()) + " " + resultValue + ", 0");
                if (stmt->guardCondition->matchKind == "Ok") {
                    condition = tag;
                } else if (stmt->guardCondition->matchKind == "Err") {
                    condition = newTemp("guard.not");
                    body.push_back("  " + condition + " = xor i1 " + tag + ", true");
                } else {
                    unsupported("unknown Result guard match kind", stmt->line);
                }
                isResultMatch = true;
            } else {
                condition = emitExpr(stmt->guardCondition.get());
            }

            body.push_back("  br i1 " + condition + ", label %" + thenLabel + ", label %" + mergeLabel);
            body.push_back(thenLabel + ":");
            pushScope();

            if (isResultMatch && stmt->guardCondition->bindingName.size() > 0) {
                const bool isOk = stmt->guardCondition->matchKind == "Ok";
                const auto* payloadType = resultOperand->checkedType->generics[isOk ? 0 : 1].get();
                const bool borrowedPayload = !TypeChecker::isCopyType(payloadType);
                ownedLocalTypes.push_back(cloneType(payloadType));
                auto* bindingType = ownedLocalTypes.back().get();
                const auto address = "%" + stmt->guardCondition->bindingName + ".guard.addr" + std::to_string(labelCounter++);
                if (borrowedPayload) {
                    bindingType->isReference = true;
                    bindingType->isMutable = false;
                    const auto payloadAddr = newTemp("guard.payload.addr");
                    if (resultOperand->kind == ExprKind::Identifier) {
                        auto* owner = lookupLocal(resultOperand->strValue);
                        if (!owner) unsupported("guard Result owner is not a local", stmt->line);
                        const auto resultType = llvmType(resultOperand->checkedType.get());
                        body.push_back("  " + payloadAddr + " = getelementptr inbounds " + resultType + ", ptr " + owner->addr + ", i32 0, i32 " + (isOk ? "1" : "2"));
                    } else {
                        const auto resultAddr = newTemp("guard.result.addr");
                        body.push_back("  " + resultAddr + " = alloca " + llvmType(resultOperand->checkedType.get()));
                        body.push_back("  store " + llvmType(resultOperand->checkedType.get()) + " " + resultValue + ", ptr " + resultAddr);
                        const auto resultType = llvmType(resultOperand->checkedType.get());
                        body.push_back("  " + payloadAddr + " = getelementptr inbounds " + resultType + ", ptr " + resultAddr + ", i32 0, i32 " + (isOk ? "1" : "2"));
                    }
                    body.push_back("  " + address + " = alloca ptr");
                    body.push_back("  store ptr " + payloadAddr + ", ptr " + address);
                } else {
                    const auto payload = newTemp("guard.payload");
                    body.push_back("  " + payload + " = extractvalue " + llvmType(resultOperand->checkedType.get()) + " " + resultValue + ", " + (isOk ? "1" : "2"));
                    body.push_back("  " + address + " = alloca " + llvmType(bindingType));
                    body.push_back("  store " + llvmType(bindingType) + " " + payload + ", ptr " + address);
                }
                localScopes.back()[stmt->guardCondition->bindingName] = LocalBinding{bindingType, address, false, false};
                scopeOrder.back().push_back(stmt->guardCondition->bindingName);
            }

            // The guard body is represented by a single Stmt node in the AST. Emit it
            // directly so a `return` correctly terminates only this predecessor.
            bool guardTerminated = false;
            if (stmt->guardBody) {
                emitStmt(stmt->guardBody.get());
                guardTerminated = blockTerminated;
            }
            if (!guardTerminated) cleanupCurrentScope();
            popScope();
            if (!guardTerminated) body.push_back("  br label %" + mergeLabel);
            body.push_back(mergeLabel + ":");
            blockTerminated = false;
            break;
        }

        case StmtKind::Break:
            if (loopLabels.empty()) unsupported("break outside loop", stmt->line);
            cleanupScopesFrom(loopLabels.back().cleanupDepth);
            body.push_back("  br label %" + loopLabels.back().exitLabel);
            blockTerminated = true;
            break;

        case StmtKind::Continue:
            if (loopLabels.empty()) unsupported("continue outside loop", stmt->line);
            cleanupScopesFrom(loopLabels.back().cleanupDepth);
            body.push_back("  br label %" + loopLabels.back().continueLabel);
            blockTerminated = true;
            break;

        case StmtKind::If: {
            const auto condition = emitExpr(stmt->expr.get());
            const auto thenLabel = newLabel("if.then");
            const auto elseLabel = newLabel("if.else");
            const auto mergeLabel = newLabel("if.end");
            body.push_back("  br i1 " + condition + ", label %" + thenLabel + ", label %" + elseLabel);

            body.push_back(thenLabel + ":");
            pushScope();
            const bool thenTerminated = emitBlock(stmt->body);
            if (!thenTerminated) cleanupCurrentScope();
            popScope();
            if (!thenTerminated) body.push_back("  br label %" + mergeLabel);

            body.push_back(elseLabel + ":");
            bool elseTerminated = false;
            if (!stmt->elseBody.empty()) {
                pushScope();
                elseTerminated = emitBlock(stmt->elseBody);
                if (!elseTerminated) cleanupCurrentScope();
                popScope();
            }
            if (!elseTerminated) body.push_back("  br label %" + mergeLabel);
            blockTerminated = thenTerminated && !stmt->elseBody.empty() && elseTerminated;
            // When both branches terminate, mergeLabel has no predecessor and no
            // instructions; emitting it would produce an invalid empty basic block
            // (LLVM requires every block to end in a terminator). Skip it entirely
            // since nothing else references this freshly-minted label.
            if (!blockTerminated) body.push_back(mergeLabel + ":");
            break;
        }

        case StmtKind::While: {
            const auto conditionLabel = newLabel("while.cond");
            const auto bodyLabel = newLabel("while.body");
            const auto exitLabel = newLabel("while.end");
            body.push_back("  br label %" + conditionLabel);
            body.push_back(conditionLabel + ":");
            const auto condition = emitExpr(stmt->expr.get());
            body.push_back("  br i1 " + condition + ", label %" + bodyLabel + ", label %" + exitLabel);
            body.push_back(bodyLabel + ":");
            loopLabels.push_back(LoopContext{conditionLabel, conditionLabel, exitLabel, localScopes.size()});
            pushScope();
            const bool terminated = emitBlock(stmt->body);
            if (!terminated) cleanupCurrentScope();
            popScope();
            loopLabels.pop_back();
            if (!terminated) body.push_back("  br label %" + conditionLabel);
            body.push_back(exitLabel + ":");
            blockTerminated = false;
            break;
        }

        case StmtKind::For: {
            const auto* iterableTypeRaw = stmt->iterable->checkedType.get();
            if (!iterableTypeRaw) unsupported("for iterable has no type", stmt->line);
            auto iterableType = cloneType(iterableTypeRaw);
            if (iterableType->isReference) iterableType->isReference = false;
            const bool fixed = iterableType->isArray && iterableType->fixedArraySize.has_value();
            const bool dynamic = iterableType->isArray && !iterableType->fixedArraySize.has_value();
            const bool view = iterableType->name == "View" || iterableType->name == "EditView";
            if (!fixed && !dynamic && !view) unsupported("for iterable is not a collection", stmt->line);

            std::string base;
            std::string length;
            if (fixed) {
                const auto* local = stmt->iterable->kind == ExprKind::Identifier ? lookupLocal(stmt->iterable->strValue) : nullptr;
                if (iterableTypeRaw->isReference) base = emitExpr(stmt->iterable.get());
                else if (local) base = local->addr;
                else unsupported("fixed-array for loops currently require a local array", stmt->line);
                length = std::to_string(*iterableType->fixedArraySize);
            } else if (dynamic) {
                const auto arrayValue = iterableTypeRaw->isReference ? [&]() {
                    const auto p = emitExpr(stmt->iterable.get());
                    const auto loaded = newTemp("for.array.ref");
                    body.push_back("  " + loaded + " = load %LannerDynArray, ptr " + p);
                    return loaded;
                }() : emitExpr(stmt->iterable.get());
                base = newTemp("for.array.base");
                length = newTemp("for.array.len");
                body.push_back("  " + base + " = extractvalue %LannerDynArray " + arrayValue + ", 0");
                body.push_back("  " + length + " = extractvalue %LannerDynArray " + arrayValue + ", 1");
            } else {
                const auto viewValue = iterableTypeRaw->isReference ? [&]() {
                    const auto p = emitExpr(stmt->iterable.get());
                    const auto loaded = newTemp("for.view.ref");
                    body.push_back("  " + loaded + " = load { ptr, i64 }, ptr " + p);
                    return loaded;
                }() : emitExpr(stmt->iterable.get());
                base = newTemp("for.view.base");
                length = newTemp("for.view.len");
                body.push_back("  " + base + " = extractvalue { ptr, i64 } " + viewValue + ", 0");
                body.push_back("  " + length + " = extractvalue { ptr, i64 } " + viewValue + ", 1");
            }

            auto elementType = fixed ? cloneType(iterableType.get()) :
                               (dynamic ? cloneType(iterableType.get()) : cloneType(iterableType->generics[0].get()));
            elementType->isArray = false;
            elementType->fixedArraySize.reset();
            elementType->origin = {};
            elementType->isReference = false;
            elementType->isMutable = false;
            const auto elemLLVM = llvmType(elementType.get());

            const auto idxAddr = "%for.idx" + std::to_string(labelCounter++);
            body.push_back("  " + idxAddr + " = alloca i64");
            body.push_back("  store i64 0, ptr " + idxAddr);
            const auto condLabel = newLabel("for.cond");
            const auto bodyLabel = newLabel("for.body");
            const auto nextLabel = newLabel("for.next");
            const auto exitLabel = newLabel("for.end");
            body.push_back("  br label %" + condLabel);
            body.push_back(condLabel + ":");
            const auto idx = newTemp("for.idx.load");
            body.push_back("  " + idx + " = load i64, ptr " + idxAddr);
            const auto cond = newTemp("for.cond.value");
            body.push_back("  " + cond + " = icmp ult i64 " + idx + ", " + length);
            body.push_back("  br i1 " + cond + ", label %" + bodyLabel + ", label %" + exitLabel);
            body.push_back(bodyLabel + ":");

            const auto elemAddr = newTemp("for.elem.addr");
            if (fixed) {
                const auto arrayType = llvmType(iterableType.get());
                body.push_back("  " + elemAddr + " = getelementptr inbounds " + arrayType + ", ptr " + base + ", i64 0, i64 " + idx);
            } else {
                body.push_back("  " + elemAddr + " = getelementptr inbounds " + elemLLVM + ", ptr " + base + ", i64 " + idx);
            }

            loopLabels.push_back(LoopContext{condLabel, nextLabel, exitLabel, localScopes.size()});
            pushScope();
            auto loopType = cloneType(elementType.get());
            loopType->isReference = true;
            ownedLocalTypes.push_back(std::move(loopType));
            auto* loopBindingType = ownedLocalTypes.back().get();
            const auto loopAddr = "%" + stmt->loopVar + ".for.addr" + std::to_string(labelCounter++);
            body.push_back("  " + loopAddr + " = alloca ptr");
            body.push_back("  store ptr " + elemAddr + ", ptr " + loopAddr);
            localScopes.back()[stmt->loopVar] = LocalBinding{loopBindingType, loopAddr, false, false};
            scopeOrder.back().push_back(stmt->loopVar);

            const bool terminated = emitBlock(stmt->body);
            if (!terminated) cleanupCurrentScope();
            popScope();
            loopLabels.pop_back();
            if (!terminated) body.push_back("  br label %" + nextLabel);
            body.push_back(nextLabel + ":");
            const auto nextIdx = newTemp("for.idx.next");
            body.push_back("  " + nextIdx + " = add i64 " + idx + ", 1");
            body.push_back("  store i64 " + nextIdx + ", ptr " + idxAddr);
            body.push_back("  br label %" + condLabel);
            body.push_back(exitLabel + ":");
            blockTerminated = false;
            break;
        }

        default:
            unsupported("statement kind is not yet lowered", stmt->line);
    }
}

bool LLVMCodeGenerator::emitBlock(const std::vector<std::unique_ptr<Stmt>>& stmts) {
    bool terminated = false;
    blockTerminated = false;
    for (const auto& stmt : stmts) {
        if (terminated) unsupported("statement follows terminated block", stmt->line);
        emitStmt(stmt.get());
        terminated = blockTerminated;
    }
    return terminated;
}

std::string LLVMCodeGenerator::emitStaticConstant(const Expr* expr, const TypeNode* type) {
    if (!expr || !type) unsupported("invalid static constant", expr ? expr->line : 0);
    if (type->isOptional) {
        auto payload=cloneType(type); payload->isOptional=false;
        if (expr->kind == ExprKind::NoneLit) return "zeroinitializer";
        const auto payloadConst=emitStaticConstant(expr,payload.get());
        return "{ i1 1, "+llvmType(payload.get())+" "+payloadConst+" }";
    }
    if (type->isArray && type->fixedArraySize) {
        auto elem=cloneType(type); elem->isArray=false; elem->fixedArraySize.reset();
        if (expr->kind != ExprKind::ArrayLit || expr->args.size() != *type->fixedArraySize) unsupported("static fixed array initializer must have exactly the declared number of elements", expr->line);
        std::string out="[";
        for (std::size_t i=0;i<expr->args.size();++i) { if(i) out += ", "; out += llvmType(elem.get())+" "+emitStaticConstant(expr->args[i].get(),elem.get()); }
        out += "]"; return out;
    }
    if (structs.count(type->name) && expr->kind == ExprKind::StructLit) {
        const auto* st=structs.at(type->name); std::string out="{";
        for(std::size_t i=0;i<st->fields.size();++i){ if(i) out += ", "; const auto& f=st->fields[i]; const Expr* val=nullptr; for(const auto& supplied: expr->fields) if(supplied.first==f.name) val=supplied.second.get(); if(!val) unsupported("missing static struct field",expr->line); out += llvmType(f.type.get())+" "+emitStaticConstant(val,f.type.get()); }
        out += "}"; return out;
    }
    if (expr->kind == ExprKind::Identifier && expr->checkedType && expr->checkedType->name == "fn" && functions.count(expr->strValue)) return "@" + ((expr->strValue == "main" && includeRuntime && !isWebTarget()) ? std::string("lanner_user_main") : expr->strValue);
    if (expr->kind == ExprKind::NoneLit && expr->strValue == "null") return "null";
    if (expr->kind == ExprKind::IntLit) return normalizeIntLiteral(expr->strValue);
    if (expr->kind == ExprKind::BoolLit) return expr->strValue == "true" ? "1" : "0";
    if (expr->kind == ExprKind::FloatLit) return llvmFloatLiteral(expr);
    if (expr->kind == ExprKind::StringLit) {
        const auto name = ".lanner.static.str." + std::to_string(stringLiterals.size());
        std::string encoded; for(unsigned char c: expr->strValue){ if(c>=0x20&&c<=0x7e&&c!='"'&&c!='\\') encoded.push_back((char)c); else encoded += hexByte(c); } encoded += "\\00";
        stringLiterals.push_back("@"+name+" = private unnamed_addr constant ["+std::to_string(expr->strValue.size()+1)+" x i8] c\""+encoded+"\"");
        return "getelementptr inbounds (["+std::to_string(expr->strValue.size()+1)+" x i8], ptr @"+name+", i64 0, i64 0)";
    }
    if (expr->kind == ExprKind::Identifier) {
        if (const auto* c=lookupComptime(expr->strValue)) return comptimeLiteral(*c);
    }
    if (expr->kind == ExprKind::UnaryOp) {
        const auto v=emitStaticConstant(expr->value.get(), type);
        if (expr->op=="-") return "sub "+llvmType(type)+" 0, "+v;
        if (expr->op=="+") return v;
        if (expr->op=="!") return "xor i1 "+v+", true";
        if (expr->op=="~") return "xor "+llvmType(type)+" "+v+", -1";
    }
    if (expr->kind == ExprKind::BinaryOp) {
        const auto l=emitStaticConstant(expr->left.get(),type); const auto r=emitStaticConstant(expr->right.get(),type);
        if (llvmType(type)=="i1") { if(expr->op=="=="||expr->op=="!=") return "icmp "+std::string(expr->op=="=="?"eq":"ne")+" i1 "+l+", "+r; }
        const std::string ty=llvmType(type); const auto signedness=(type->name.size()&&type->name[0]=='i');
        if (expr->op == "+") return "add " + ty + " " + l + ", " + r;
        if (expr->op == "-") return "sub " + ty + " " + l + ", " + r;
        if (expr->op == "*") return "mul " + ty + " " + l + ", " + r;
        if (expr->op == "/") return std::string(signedness ? "sdiv " : "udiv ") + ty + " " + l + ", " + r;
        if (expr->op == "&" || expr->op == "|" || expr->op == "^") {
            return std::string(expr->op == "&" ? "and " : expr->op == "|" ? "or " : "xor ") + ty + " " + l + ", " + r;
        }
    }
    unsupported("unsupported static initializer expression", expr->line);
}

void LLVMCodeGenerator::emitStaticDecl(const StaticDecl& decl, std::string& out) {
    const auto ty = llvmType(decl.type.get());
    if (decl.isExtern) {
        out += "@" + decl.name + " = external " + (decl.isThreadLocal ? "thread_local " : "") +
               (decl.isMutable ? "global " : "constant ") + ty;
        if (decl.alignment) out += ", align " + std::to_string(decl.alignment);
        out += "\n";
        return;
    }
    const auto value=emitStaticConstant(decl.value.get(),decl.type.get());
    out += "@"+decl.name+" = dso_local "+(decl.isThreadLocal?"thread_local ":"")+(decl.isMutable?"global ":"constant ")+ty+" "+value;
    if (!decl.section.empty()) out += ", section \"" + decl.section + "\"";
    if (decl.alignment) out += ", align " + std::to_string(decl.alignment);
    out += "\n";
}

void LLVMCodeGenerator::emitThreadWrapper(const FunctionDecl& fn, std::string& out) {
    if (fn.isExtern || !fn.params.empty() || llvmType(fn.returnType.get()) != "void") return;
    out += "define internal void @__lanner_thread_entry_"+fn.name+"(ptr %ctx) {\nentry:\n  call void @"+fn.name+"()\n  ret void\n}\n\n";
}

void LLVMCodeGenerator::emitFunction(const FunctionDecl& fn, std::string& out) {
    if (fn.isExtern) return;
    currentFunction = &fn;
    localScopes.clear();
    scopeOrder.clear();
    ownedLocalTypes.clear();
    loopLabels.clear();
    body.clear();
    blockTerminated = false;
    tempCounter = 0;
    labelCounter = 0;

    pushScope();
    const auto ret = llvmType(fn.returnType.get());
    const std::string emittedName = (fn.name == "main" && includeRuntime && !isWebTarget()) ? "lanner_user_main" : fn.name;
    out += "define " + ret + " @" + emittedName + "(";
    for (std::size_t i = 0; i < fn.params.size(); ++i) {
        if (i) out += ", ";
        out += llvmType(fn.params[i].type.get()) + " %" + fn.params[i].name;
    }
    out += ") {\nentry:\n";

    for (const auto& p : fn.params) {
        const auto address = "%" + p.name + ".param.addr";
        localScopes.back()[p.name] = LocalBinding{p.type.get(), address};
        scopeOrder.back().push_back(p.name);
        body.push_back("  " + address + " = alloca " + llvmType(p.type.get()));
        body.push_back("  store " + llvmType(p.type.get()) + " %" + p.name + ", ptr " + address);
    }

    emitBlock(fn.body);
    if (!blockTerminated) {
        cleanupAllScopes();
        if (ret == "void") body.push_back("  ret void");
        else if (ret == "i1") body.push_back("  ret i1 0");
        else if (ret.rfind("i", 0) == 0) body.push_back("  ret " + ret + " 0");
        else body.push_back("  ret " + ret + " zeroinitializer");
    }

    for (const auto& line : body) out += line + "\n";
    out += "}\n\n";
    popScope();
}

std::string LLVMCodeGenerator::generate(const Program& program, bool runtime, const std::string& triple) {
    includeRuntime = runtime;
    targetTriple = triple;
    pointerBits = pointerBitsForTriple(triple);
    functions.clear();
    structs.clear();
    enums.clear();
    statics.clear();
    comptimeGlobals.clear();
    comptimeScopes.clear();
    stringLiterals.clear();
    for (const auto& decl : program.decls) {
        if (decl->kind == DeclKind::Function) functions[decl->fn->name] = decl->fn.get();
        if (decl->kind == DeclKind::Struct) structs[decl->st->name] = decl->st.get();
        if (decl->kind == DeclKind::Enum) enums[decl->en->name] = decl->en.get();
        if (decl->kind == DeclKind::Static) statics[decl->staticDecl->name] = decl->staticDecl.get();
    }
    // Comptime globals carry no runtime storage, so the direct LLVM backend
    // must fold them to literal constants up front (mirrors what the
    // typechecker's ComptimeEvaluator already proved is a valid constant
    // expression) rather than treating their identifier as a local slot.
    {
        SymbolTable comptimeSymbols;
        ComptimeEvaluator evaluator(comptimeSymbols);
        for (const auto& decl : program.decls) {
            if (decl->kind != DeclKind::ComptimeGlobal) continue;
            evaluator.evaluateAndDeclare(decl->comptimeGlobal.get());
            if (const Symbol* s = comptimeSymbols.resolve(decl->comptimeGlobal->comptimeName)) {
                comptimeGlobals[decl->comptimeGlobal->comptimeName] = *s->comptimeValue;
            }
        }
    }

    std::string out;
    out += "; Lanner LLVM IR\n";
    out += "; generated by lanner\n";
    if (!targetTriple.empty()) out += "target triple = \"" + targetTriple + "\"\n";
    out += "\n";
    out += "%LannerArena = type { ptr, ptr, i64, i64 }\n";
    out += "%LannerDynArray = type { ptr, i64, i64, ptr }\n";
    out += "%LannerArenaNode = type { ptr, i8 }\n\n";
    out += "declare void @llvm.trap()\n";
    out += "declare void @llvm.assume(i1)\n";
    out += "declare ptr @malloc(i64)\n";
    out += "declare ptr @realloc(ptr, i64)\n";
    out += "declare ptr @lanner_aligned_alloc(i64, i64)\n";
    out += "declare void @lanner_aligned_free(ptr)\n";
    out += "declare void @free(ptr)\n";
    out += "declare ptr @memset(ptr, i32, i64)\n";
    out += "declare ptr @memcpy(ptr, ptr, i64)\n";
    out += "declare ptr @memmove(ptr, ptr, i64)\n";
    out += "declare i32 @memcmp(ptr, ptr, i64)\n";
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
    out += "declare i32 @printf(ptr, ...)\n";
    {
        out += "declare ptr @__lanner_process_argv_at(i64)\n";
        out += "declare i64 @__lanner_process_argc()\n";
        out += "declare i32 @__lanner_env_has(ptr)\n";
        out += "declare i32 @__lanner_set_env_value(ptr, ptr)\n";
        out += "declare i32 @__lanner_set_env_unset(ptr)\n";
        out += "declare i32 @__lanner_fs_exists(ptr)\n";
        out += "declare i32 @__lanner_fs_isFile(ptr)\n";
        out += "declare i32 @__lanner_fs_isDir(ptr)\n";
        out += "declare i64 @__lanner_fs_file_size(ptr)\n";
        out += "declare ptr @__lanner_fs_read(ptr)\n";
        out += "declare i32 @__lanner_fs_write(ptr, ptr)\n";
        out += "declare i32 @__lanner_fs_append(ptr, ptr)\n";
        out += "declare i32 @__lanner_fs_write_buffer(ptr, ptr, i64)\n";
        out += "declare i32 @__lanner_fs_remove(ptr)\n";
        out += "declare i32 @__lanner_fs_mkdir(ptr)\n";
        out += "declare i32 @__lanner_fs_rmdir(ptr)\n";
        out += "declare i32 @__lanner_fs_rename(ptr, ptr)\n";
        out += "declare i32 @__lanner_fs_copy(ptr, ptr)\n";
        out += "declare ptr @__lanner_fs_cwd()\n";
        out += "declare i32 @__lanner_fs_chdir(ptr)\n";
        out += "declare ptr @__lanner_fs_list(ptr)\n";
        out += "declare ptr @__lanner_path_join(ptr, ptr)\n";
        out += "declare ptr @__lanner_path_basename(ptr)\n";
        out += "declare ptr @__lanner_path_dirname(ptr)\n";
        out += "declare ptr @__lanner_path_extension(ptr)\n";
        out += "declare ptr @__lanner_path_stem(ptr)\n";
        out += "declare ptr @__lanner_path_normalize(ptr)\n";
        out += "declare ptr @__lanner_path_absolute(ptr)\n";
        out += "declare i32 @__lanner_path_is_absolute(ptr)\n";
        out += "declare ptr @__lanner_regex_compile(ptr)\n";
        out += "declare i32 @__lanner_regex_match(ptr, ptr)\n";
        out += "declare i64 @__lanner_regex_find(ptr, ptr)\n";
        out += "declare void @__lanner_regex_free(ptr)\n";
        out += "declare ptr @__lanner_shell_which(ptr)\n";
        out += "declare i64 @__lanner_string_parse_i64(ptr)\n";
        out += "declare double @__lanner_string_parse_f64(ptr)\n";
        out += "declare i32 @__lanner_string_contains(ptr, ptr)\n";
        out += "declare i32 @__lanner_string_startsWith(ptr, ptr)\n";
        out += "declare i32 @__lanner_string_endsWith(ptr, ptr)\n";
        out += "declare i32 @__lanner_string_equalsIgnoreCase(ptr, ptr)\n";
        out += "declare i64 @__lanner_string_find(ptr, ptr)\n";
    }
    {
        out += "declare ptr @__lanner_net_tcp_connect(ptr, i32, i32)\n";
        out += "declare ptr @__lanner_net_tcp_listen(ptr, i32, i32)\n";
        out += "declare ptr @__lanner_net_accept(ptr)\n";
        out += "declare void @__lanner_net_close(ptr)\n";
        out += "declare i64 @__lanner_net_send(ptr, ptr, i64)\n";
        out += "declare i64 @__lanner_net_recv(ptr, ptr, i64)\n";
        out += "declare i64 @__lanner_net_send_string(ptr, ptr)\n";
        out += "declare i32 @__lanner_net_set_nonblocking(ptr, i1)\n";
        out += "declare i32 @__lanner_net_poll(ptr, i32, i32)\n";
        out += "declare i32 @__lanner_net_tcp_nodelay(ptr, i1)\n";
        out += "declare i32 @__lanner_net_shutdown(ptr, i32)\n";
        out += "declare i32 @__lanner_net_last_error()\n";
        out += "declare ptr @__lanner_net_error_string()\n";
        out += "declare ptr @__lanner_net_udp_open()\n";
        out += "declare i32 @__lanner_net_udp_bind(ptr, ptr, i32)\n";
        out += "declare i64 @__lanner_net_udp_send_to(ptr, ptr, i32, ptr, i64)\n";
        out += "declare i64 @__lanner_net_udp_recv(ptr, ptr, i64)\n";
        out += "declare i32 @__lanner_net_local_port(ptr)\n";
        out += "declare i32 @__lanner_net_peer_port(ptr)\n";
        out += "declare ptr @__lanner_poller_create()\n";
        out += "declare i32 @__lanner_poller_add(ptr, ptr, i32)\n";
        out += "declare i32 @__lanner_poller_remove(ptr, ptr)\n";
        out += "declare i32 @__lanner_poller_wait(ptr, i32)\n";
        out += "declare i32 @__lanner_poller_count(ptr)\n";
        out += "declare ptr @__lanner_poller_event_socket(ptr, i32)\n";
        out += "declare i32 @__lanner_poller_event_mask(ptr, i32)\n";
        out += "declare i32 @__lanner_poller_read_events()\n";
        out += "declare i32 @__lanner_poller_write_events()\n";
        out += "declare i32 @__lanner_poller_error_events()\n";
        out += "declare void @__lanner_poller_destroy(ptr)\n";
        out += "declare ptr @__lanner_mutex_create()\n";
        out += "declare void @__lanner_mutex_lock(ptr)\n";
        out += "declare i32 @__lanner_mutex_try_lock(ptr)\n";
        out += "declare void @__lanner_mutex_unlock(ptr)\n";
        out += "declare void @__lanner_mutex_destroy(ptr)\n";
        out += "declare ptr @__lanner_rwlock_create()\n";
        out += "declare void @__lanner_rwlock_read_lock(ptr)\n";
        out += "declare void @__lanner_rwlock_write_lock(ptr)\n";
        out += "declare i32 @__lanner_rwlock_try_read_lock(ptr)\n";
        out += "declare i32 @__lanner_rwlock_try_write_lock(ptr)\n";
        out += "declare void @__lanner_rwlock_unlock(ptr, i1)\n";
        out += "declare void @__lanner_rwlock_destroy(ptr)\n";
        out += "declare ptr @__lanner_condvar_create()\n";
        out += "declare i32 @__lanner_condvar_wait(ptr, ptr, i32)\n";
        out += "declare void @__lanner_condvar_signal(ptr)\n";
        out += "declare void @__lanner_condvar_broadcast(ptr)\n";
        out += "declare void @__lanner_condvar_destroy(ptr)\n";
        out += "declare ptr @__lanner_semaphore_create(i32)\n";
        out += "declare i32 @__lanner_semaphore_wait(ptr, i32)\n";
        out += "declare i32 @__lanner_semaphore_try_wait(ptr)\n";
        out += "declare void @__lanner_semaphore_post(ptr)\n";
        out += "declare void @__lanner_semaphore_destroy(ptr)\n";
        out += "declare i32 @__lanner_process_run(ptr)\n";
        out += "declare ptr @__lanner_process_spawn(ptr)\n";
        out += "declare i32 @__lanner_process_wait(ptr)\n";
        out += "declare i64 @__lanner_process_pid(ptr)\n";
        out += "declare i32 @__lanner_process_terminate(ptr)\n";
        out += "declare ptr @__lanner_process_output(ptr)\n";
        out += "declare i32 @__lanner_set_env(ptr, ptr)\n";
        out += "declare ptr @__lanner_buffer_new(i64)\n";
        out += "declare ptr @__lanner_buffer_from_string(ptr)\n";
        out += "declare i64 @__lanner_buffer_len(ptr)\n";
        out += "declare ptr @__lanner_buffer_data(ptr)\n";
        out += "declare ptr @__lanner_buffer_cstr(ptr)\n";
        out += "declare void @__lanner_buffer_free(ptr)\n";
        out += "declare i32 @__lanner_buffer_append_string(ptr, ptr)\n";
        out += "declare i32 @__lanner_buffer_append_buffer(ptr, ptr)\n";
        out += "declare ptr @__lanner_http_get(ptr, i32)\n";
        out += "declare ptr @__lanner_http_post(ptr, ptr, i32)\n";
        out += "declare i32 @__lanner_http_status()\n";
        out += "declare ptr @__lanner_game_window_create(ptr, i32, i32, i32)\n";
        out += "declare void @__lanner_game_window_destroy(ptr)\n";
        out += "declare i32 @__lanner_game_poll(ptr)\n";
        out += "declare i32 @__lanner_game_should_close(ptr)\n";
        out += "declare void @__lanner_game_request_close(ptr)\n";
        out += "declare void @__lanner_game_set_title(ptr, ptr)\n";
        out += "declare i32 @__lanner_game_window_width(ptr)\n";
        out += "declare i32 @__lanner_game_window_height(ptr)\n";
        out += "declare i32 @__lanner_game_set_vsync(ptr, i1)\n";
        out += "declare i32 @__lanner_game_make_gl_context(ptr)\n";
        out += "declare void @__lanner_game_present(ptr)\n";
        out += "declare i32 @__lanner_game_window_flags(i1, i1, i1, i1)\n";
        out += "declare i32 @__lanner_game_renderer_flags(i1, i1)\n";
        out += "declare ptr @__lanner_game_renderer_create(ptr, i32)\n";
        out += "declare void @__lanner_game_renderer_destroy(ptr)\n";
        out += "declare i32 @__lanner_game_renderer_set_color(ptr, i8, i8, i8, i8)\n";
        out += "declare i32 @__lanner_game_renderer_clear(ptr)\n";
        out += "declare i32 @__lanner_game_renderer_line(ptr, i32, i32, i32, i32)\n";
        out += "declare i32 @__lanner_game_renderer_fill_rect(ptr, i32, i32, i32, i32)\n";
        out += "declare void @__lanner_game_renderer_present(ptr)\n";
        out += "declare ptr @__lanner_game_texture_create(ptr, i32, i32, i32, i32)\n";
        out += "declare i32 @__lanner_game_texture_update(ptr, ptr, i32)\n";
        out += "declare i32 @__lanner_game_texture_copy(ptr, ptr, i32, i32, i32)\n";
        out += "declare void @__lanner_game_texture_destroy(ptr)\n";
        out += "declare i32 @__lanner_game_event_type()\n";
        out += "declare i32 @__lanner_game_event_code()\n";
        out += "declare i32 @__lanner_game_event_x()\n";
        out += "declare i32 @__lanner_game_event_y()\n";
        out += "declare ptr @__lanner_game_event_text()\n";
        out += "declare i32 @__lanner_game_key_down(i32)\n";
        out += "declare i32 @__lanner_game_mouse_button_down(i32)\n";
        out += "declare i32 @__lanner_game_mouse_x()\n";
        out += "declare i32 @__lanner_game_mouse_y()\n";
        out += "declare i32 @__lanner_game_controller_connected(i32)\n";
        out += "declare float @__lanner_game_controller_axis(i32, i32)\n";
        out += "declare i32 @__lanner_game_controller_button(i32, i32)\n";
        out += "declare ptr @__lanner_game_audio_open(i32, i32, i32)\n";
        out += "declare i64 @__lanner_game_audio_write(ptr, ptr, i64)\n";
        out += "declare i64 @__lanner_game_audio_queued(ptr)\n";
        out += "declare void @__lanner_game_audio_pause(ptr, i1)\n";
        out += "declare void @__lanner_game_audio_close(ptr)\n";
        out += "declare i64 @__lanner_game_time_nanos()\n";
        out += "declare double @__lanner_game_delta_seconds()\n";
        out += "declare void @__lanner_game_sleep_nanos(i64)\n";
        out += "declare i32 @__lanner_gfx_available(ptr)\n";
        out += "declare ptr @__lanner_gfx_backend()\n";
        out += "declare ptr @__lanner_gfx_load_proc(ptr, ptr)\n";
        out += "declare void @__lanner_gl_clear_color(float, float, float, float)\n";
        out += "declare void @__lanner_gl_clear(i32)\n";
        out += "declare void @__lanner_gl_viewport(i32, i32, i32, i32)\n";
        out += "declare void @__lanner_gl_enable(i32)\n";
        out += "declare void @__lanner_gl_disable(i32)\n";
        out += "declare void @__lanner_gl_gen_buffers(i32, ptr)\n";
        out += "declare void @__lanner_gl_gen_vertex_arrays(i32, ptr)\n";
        out += "declare void @__lanner_gl_bind_buffer(i32, i32)\n";
        out += "declare void @__lanner_gl_buffer_data(i32, i64, ptr, i32)\n";
        out += "declare i32 @__lanner_gl_create_shader(i32)\n";
        out += "declare void @__lanner_gl_shader_source(i32, ptr)\n";
        out += "declare void @__lanner_gl_compile_shader(i32)\n";
        out += "declare i32 @__lanner_gl_shader_status(i32)\n";
        out += "declare ptr @__lanner_gl_shader_log(i32)\n";
        out += "declare void @__lanner_gl_attach_shader(i32, i32)\n";
        out += "declare void @__lanner_gl_link_program(i32)\n";
        out += "declare i32 @__lanner_gl_create_program()\n";
        out += "declare i32 @__lanner_gl_program_status(i32)\n";
        out += "declare ptr @__lanner_gl_program_log(i32)\n";
        out += "declare void @__lanner_gl_use_program(i32)\n";
        out += "declare void @__lanner_gl_draw_arrays(i32, i32, i32)\n";
        out += "declare void @__lanner_gl_bind_vertex_array(i32)\n";
        out += "declare void @__lanner_gl_enable_vertex_attrib(i32)\n";
        out += "declare void @__lanner_gl_vertex_attrib_pointer(i32, i32, i32, i1, i32, i64)\n";
        out += "declare i32 @__lanner_gl_get_error()\n";
        out += "declare void @__lanner_gl_delete_shader(i32)\n";
        out += "declare void @__lanner_gl_delete_program(i32)\n";
        out += "declare void @__lanner_gl_delete_buffers(i32, ptr)\n";
        out += "declare void @__lanner_gl_delete_vertex_arrays(i32, ptr)\n";
        out += "declare i32 @__lanner_json_validate(ptr)\n";
        out += "declare ptr @__lanner_json_quote(ptr)\n";
        out += "declare ptr @__lanner_json_int(i64)\n";
        out += "declare ptr @__lanner_json_float(double)\n";
        out += "declare ptr @__lanner_json_bool(i1)\n";
        out += "declare ptr @__lanner_json_null()\n";
        out += "declare void @__lanner_process_set_argv(i32, ptr)\n";
        out += "declare ptr @__lanner_tensor_zeros1(i64)\n";
        out += "declare ptr @__lanner_tensor_zeros2(i64, i64)\n";
        out += "declare ptr @__lanner_tensor_zeros3(i64, i64, i64)\n";
        out += "declare ptr @__lanner_tensor_zeros4(i64, i64, i64, i64)\n";
        out += "declare ptr @__lanner_tensor_ones1(i64)\n";
        out += "declare ptr @__lanner_tensor_ones2(i64, i64)\n";
        out += "declare ptr @__lanner_tensor_ones3(i64, i64, i64)\n";
        out += "declare ptr @__lanner_tensor_ones4(i64, i64, i64, i64)\n";
        out += "declare ptr @__lanner_tensor_zeros_f32(i64)\n";
        out += "declare ptr @__lanner_tensor_ones_f32(i64)\n";
        out += "declare ptr @__lanner_tensor_zeros_f64(i64)\n";
        out += "declare ptr @__lanner_tensor_ones_f64(i64)\n";
        out += "declare ptr @__lanner_tensor_from1_f32(ptr, i64)\n";
        out += "declare ptr @__lanner_tensor_from1_f64(ptr, i64)\n";
        out += "declare ptr @__lanner_tensor_from2_f32(ptr, i64, i64)\n";
        out += "declare ptr @__lanner_tensor_from2_f64(ptr, i64, i64)\n";
        out += "declare ptr @__lanner_tensor_clone(ptr)\n";
        out += "declare ptr @__lanner_tensor_contiguous(ptr)\n";
        out += "declare void @__lanner_tensor_free(ptr)\n";
        out += "declare i64 @__lanner_tensor_rank(ptr)\n";
        out += "declare i64 @__lanner_tensor_len(ptr)\n";
        out += "declare i64 @__lanner_tensor_dim(ptr, i64)\n";
        out += "declare i64 @__lanner_tensor_stride(ptr, i64)\n";
        out += "declare i32 @__lanner_tensor_dtype(ptr)\n";
        out += "declare i32 @__lanner_tensor_is_contiguous(ptr)\n";
        out += "declare ptr @__lanner_tensor_data_f32(ptr)\n";
        out += "declare ptr @__lanner_tensor_data_f64(ptr)\n";
        out += "declare double @__lanner_tensor_get1(ptr, i64)\n";
        out += "declare double @__lanner_tensor_get2(ptr, i64, i64)\n";
        out += "declare double @__lanner_tensor_get3(ptr, i64, i64, i64)\n";
        out += "declare void @__lanner_tensor_set1(ptr, i64, double)\n";
        out += "declare void @__lanner_tensor_set2(ptr, i64, i64, double)\n";
        out += "declare void @__lanner_tensor_set3(ptr, i64, i64, i64, double)\n";
        out += "declare ptr @__lanner_tensor_add(ptr, ptr)\n";
        out += "declare ptr @__lanner_tensor_sub(ptr, ptr)\n";
        out += "declare ptr @__lanner_tensor_mul(ptr, ptr)\n";
        out += "declare ptr @__lanner_tensor_div(ptr, ptr)\n";
        out += "declare ptr @__lanner_tensor_matmul(ptr, ptr)\n";
        out += "declare ptr @__lanner_tensor_scale(ptr, double)\n";
        out += "declare ptr @__lanner_tensor_relu(ptr)\n";
        out += "declare ptr @__lanner_tensor_sigmoid(ptr)\n";
        out += "declare ptr @__lanner_tensor_tanh(ptr)\n";
        out += "declare ptr @__lanner_tensor_softmax(ptr, i64)\n";
        out += "declare double @__lanner_tensor_sum(ptr)\n";
        out += "declare double @__lanner_tensor_mean(ptr)\n";
        out += "declare double @__lanner_tensor_l2norm(ptr)\n";
        out += "declare double @__lanner_tensor_dot(ptr, ptr)\n";
        out += "declare i64 @__lanner_tensor_argmax(ptr, i64)\n";
        out += "declare ptr @__lanner_tensor_reshape2(ptr, i64, i64)\n";
        out += "declare ptr @__lanner_tensor_reshape3(ptr, i64, i64, i64)\n";
        out += "declare ptr @__lanner_tensor_reshape4(ptr, i64, i64, i64, i64)\n";
        out += "declare ptr @__lanner_tensor_transpose2(ptr)\n";
        out += "declare ptr @__lanner_tensor_slice(ptr, i64, i64, i64, i64)\n";
        out += "declare void @__lanner_tensor_fill(ptr, double)\n";
        out += "declare ptr @__lanner_tensor_conv2d(ptr, ptr, i64, i64)\n";
        out += "declare ptr @__lanner_grad_create()\n";
        out += "declare void @__lanner_grad_watch(ptr, ptr)\n";
        out += "declare ptr @__lanner_grad_add(ptr, ptr, ptr)\n";
        out += "declare ptr @__lanner_grad_mul(ptr, ptr, ptr)\n";
        out += "declare ptr @__lanner_grad_matmul(ptr, ptr, ptr)\n";
        out += "declare ptr @__lanner_grad_relu(ptr, ptr)\n";
        out += "declare ptr @__lanner_grad_tanh(ptr, ptr)\n";
        out += "declare ptr @__lanner_grad_sum(ptr, ptr)\n";
        out += "declare ptr @__lanner_grad_scale(ptr, ptr, double)\n";
        out += "declare void @__lanner_grad_backward(ptr, ptr)\n";
        out += "declare ptr @__lanner_grad_get(ptr, ptr)\n";
        out += "declare void @__lanner_grad_free(ptr)\n";
        out += "declare i32 @__lanner_accel_cuda_available()\n";
        out += "declare i32 @__lanner_accel_rocm_available()\n";
        out += "declare i32 @__lanner_accel_metal_available()\n";
        out += "declare i32 @__lanner_accel_blas_available()\n";
        out += "declare ptr @__lanner_accel_backend()\n";
    }
    if (targetTriple.find("android") != std::string::npos || targetTriple.find("apple-ios") != std::string::npos) {
        out += "declare void @__lanner_mobile_log(ptr)\n";
        out += "declare ptr @__lanner_mobile_platform()\n";
        out += "declare ptr @__lanner_mobile_os_version()\n";
        out += "declare i32 @__lanner_mobile_is_simulator()\n";
        out += "declare i32 @__lanner_mobile_screen_width()\n";
        out += "declare i32 @__lanner_mobile_screen_height()\n";
        out += "declare double @__lanner_mobile_device_scale()\n";
        out += "declare i32 @__lanner_mobile_safe_top()\n";
        out += "declare i32 @__lanner_mobile_safe_bottom()\n";
        out += "declare i32 @__lanner_mobile_safe_left()\n";
        out += "declare i32 @__lanner_mobile_safe_right()\n";
        out += "declare i32 @__lanner_mobile_open_url(ptr)\n";
        out += "declare i32 @__lanner_mobile_vibrate(i32)\n";
        out += "declare i32 @__lanner_mobile_request_permission(ptr)\n";
        out += "declare i32 @__lanner_mobile_clipboard_set(ptr)\n";
        out += "declare ptr @__lanner_mobile_clipboard_get()\n";
        out += "declare i32 @__lanner_mobile_camera_available()\n";
        out += "declare i32 @__lanner_mobile_location_available()\n";
        out += "declare i32 @__lanner_mobile_bluetooth_available()\n";
        out += "declare ptr @__lanner_mobile_app_data_path()\n";
        out += "declare ptr @__lanner_mobile_documents_path()\n";
        out += "declare ptr @__lanner_mobile_cache_path()\n";
    }
    if (isWebTarget()) {
        out += "declare void @__lanner_web_log(ptr)\n";
        out += "declare void @__lanner_web_warn(ptr)\n";
        out += "declare void @__lanner_web_error(ptr)\n";
        out += "declare void @__lanner_web_log_bool(i1)\n";
        out += "declare void @__lanner_web_log_i64(i64)\n";
        out += "declare void @__lanner_web_log_f64(double)\n";
        out += "declare double @__lanner_web_now_ms()\n";
        out += "declare double @__lanner_web_random()\n";
        out += "declare i32 @__lanner_web_set_text(ptr, ptr)\n";
        out += "declare i32 @__lanner_web_set_html(ptr, ptr)\n";
        out += "declare i32 @__lanner_web_set_attr(ptr, ptr, ptr)\n";
        out += "declare i32 @__lanner_web_add_class(ptr, ptr)\n";
        out += "declare i32 @__lanner_web_remove_class(ptr, ptr)\n";
        out += "declare i32 @__lanner_web_remove(ptr)\n";
        out += "declare i32 @__lanner_web_query_count(ptr)\n";
        out += "declare i32 @__lanner_web_focus(ptr)\n";
        out += "declare i32 @__lanner_web_set_timeout(ptr, i32)\n";
        out += "declare void @__lanner_web_clear_timeout(i32)\n";
        out += "declare i32 @__lanner_web_request_animation_frame(ptr)\n";
        out += "declare void @__lanner_web_cancel_animation_frame(i32)\n";
        out += "declare i32 @__lanner_web_queue_microtask(ptr)\n";
        out += "declare i32 @__lanner_web_add_event_listener(ptr, ptr, ptr)\n";
        out += "declare void @__lanner_web_remove_event_listener(i32)\n";
        out += "declare i32 @__lanner_web_fetch_text(ptr, ptr)\n";
        out += "declare void @__lanner_web_buffer_free(ptr)\n";
        out += "declare i64 @__lanner_stdin_has_input()\n";
    }
    if (includeRuntime || targetTriple.find("android") != std::string::npos || targetTriple.find("apple-ios") != std::string::npos) {
        out += "declare ptr @__lanner_stdin_read_line()\n";
        out += "declare i32 @__lanner_stdin_has_input()\n";
        out += "declare i64 @__lanner_string_parse_u64_at(ptr, i64)\n";
        out += "declare i32 @__lanner_string_starts_with(ptr, ptr)\n";
        out += "declare i32 @__lanner_string_equals(ptr, ptr)\n";
        out += "declare i64 @__lanner_clock_monotonic_nanos()\n";
        out += "declare void @__lanner_clock_sleep_nanos(i64)\n";
        out += "declare i64 @__lanner_thread_hardware_concurrency()\n";
        out += "declare ptr @__lanner_thread_spawn(ptr)\n";
        out += "declare void @__lanner_thread_join(ptr)\n";
        out += "declare void @__lanner_thread_detach(ptr)\n";
        out += "declare void @__lanner_thread_yield()\n";
        out += "declare i32 @__lanner_cpu_has_avx2()\n";
        out += "declare i32 @__lanner_cpu_has_avx512()\n";
        out += "declare i32 @__lanner_cpu_has_sse42()\n";
        out += "declare i32 @__lanner_cpu_has_bmi2()\n";
        out += "declare i32 @__lanner_cpu_has_popcnt()\n";
        out += "declare i64 @llvm.readcyclecounter()\n";
        out += "declare i64 @__lanner_cpu_rdtsc()\n";
    }
    out += "declare i64 @llvm.x86.bmi.pext.64(i64, i64)\n";
    out += "declare i64 @llvm.x86.bmi.pdep.64(i64, i64)\n";
    out += "declare i64 @llvm.ctpop.i64(i64)\n";
    out += "declare i64 @llvm.cttz.i64(i64, i1)\n";
    out += "declare i64 @llvm.ctlz.i64(i64, i1)\n";
    out += "declare i64 @llvm.bswap.i64(i64)\n";
    out += "declare void @llvm.prefetch(ptr, i32, i32, i32)\n\n";
    for (const auto& [name, fn] : functions) {
        if (!fn->isExtern || isBuiltinExternDeclaration(name)) continue;
        out += "declare "+llvmType(fn->returnType.get())+" @"+name+"(";
        for (std::size_t i=0;i<fn->params.size();++i) { if(i) out += ", "; out += llvmType(fn->params[i].type.get()); }
        out += ")\n";
    }
    if (!functions.empty()) out += "\n";
    if (includeRuntime || isWebTarget()) {
        out += "@.lanner.rb = private unnamed_addr constant [3 x i8] c\"rb\\00\"\n";
        out += "@.lanner.print_i64 = private unnamed_addr constant [5 x i8] c\"%ld\\0A\\00\"\n";
        out += "@.lanner.print_u64 = private unnamed_addr constant [5 x i8] c\"%lu\\0A\\00\"\n";
        out += "@.lanner.print_f64 = private unnamed_addr constant [4 x i8] c\"%g\\0A\\00\"\n";
        out += "@.lanner.print_true = private unnamed_addr constant [5 x i8] c\"true\\00\"\n";
        out += "@.lanner.print_false = private unnamed_addr constant [6 x i8] c\"false\\00\"\n";
        out += "@.lanner.print_bool = private unnamed_addr constant [3 x i8] c\"%s\\00\"\n\n";
        out += "@.lanner.print_i64_raw = private unnamed_addr constant [4 x i8] c\"%ld\\00\"\n";
        out += "@stdout = external global ptr\n";
        out += "define internal %LannerDynArray @__lanner_read_file(ptr %path) {\n";
        out += "entry:\n";
        out += "  %file = call ptr @fopen(ptr %path, ptr @.lanner.rb)\n";
        out += "  %missing = icmp eq ptr %file, null\n";
        out += "  br i1 %missing, label %fail_no_close, label %seek_end\n";
        out += "seek_end:\n";
        out += "  %seek_end_rc = call i32 @fseek(ptr %file, i64 0, i32 2)\n";
        out += "  %seek_end_bad = icmp ne i32 %seek_end_rc, 0\n";
        out += "  br i1 %seek_end_bad, label %close_fail, label %size\n";
        out += "size:\n";
        out += "  %file_size = call i64 @ftell(ptr %file)\n";
        out += "  %size_bad = icmp slt i64 %file_size, 0\n";
        out += "  br i1 %size_bad, label %close_fail, label %rewind\n";
        out += "rewind:\n";
        out += "  %rewind_rc = call i32 @fseek(ptr %file, i64 0, i32 0)\n";
        out += "  %rewind_bad = icmp ne i32 %rewind_rc, 0\n";
        out += "  br i1 %rewind_bad, label %close_fail, label %alloc_size\n";
        out += "alloc_size:\n";
        out += "  %plus_one = add i64 %file_size, 1\n";
        out += "  %overflow = icmp ult i64 %plus_one, %file_size\n";
        out += "  br i1 %overflow, label %close_fail, label %malloc_data\n";
        out += "malloc_data:\n";
        out += "  %data = call ptr @malloc(i64 %plus_one)\n";
        out += "  %alloc_bad = icmp eq ptr %data, null\n";
        out += "  br i1 %alloc_bad, label %close_fail, label %read_data\n";
        out += "read_data:\n";
        out += "  %read_count = call i64 @fread(ptr %data, i64 1, i64 %file_size, ptr %file)\n";
        out += "  %read_bad = icmp ne i64 %read_count, %file_size\n";
        out += "  br i1 %read_bad, label %free_fail, label %finish\n";
        out += "finish:\n";
        out += "  %end = getelementptr inbounds i8, ptr %data, i64 %file_size\n";
        out += "  store i8 0, ptr %end\n";
        out += "  %close_rc = call i32 @fclose(ptr %file)\n";
        out += "  %a0 = insertvalue %LannerDynArray zeroinitializer, ptr %data, 0\n";
        out += "  %a1 = insertvalue %LannerDynArray %a0, i64 %file_size, 1\n";
        out += "  %a2 = insertvalue %LannerDynArray %a1, i64 %file_size, 2\n";
        out += "  %a3 = insertvalue %LannerDynArray %a2, ptr null, 3\n";
        out += "  ret %LannerDynArray %a3\n";
        out += "free_fail:\n";
        out += "  call void @free(ptr %data)\n";
        out += "  br label %close_fail\n";
        out += "close_fail:\n";
        out += "  call i32 @fclose(ptr %file)\n";
        out += "  br label %fail\n";
        out += "fail_no_close:\n";
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
        out += "define internal ptr @__lanner_arena_alloc(ptr %arena, i64 %bytes) {\n";
        out += "entry:\n";
        out += "  %rem.addr = getelementptr inbounds %LannerArena, ptr %arena, i32 0, i32 2\n";
        out += "  %cur.addr = getelementptr inbounds %LannerArena, ptr %arena, i32 0, i32 1\n";
        out += "  %head.addr = getelementptr inbounds %LannerArena, ptr %arena, i32 0, i32 0\n";
        out += "  %chunk.addr = getelementptr inbounds %LannerArena, ptr %arena, i32 0, i32 3\n";
        out += "  %remaining = load i64, ptr %rem.addr\n";
        out += "  %current = load ptr, ptr %cur.addr\n";
        out += "  %align_sum = add i64 %bytes, 7\n";
        out += "  %align_overflow = icmp ult i64 %align_sum, %bytes\n";
        out += "  br i1 %align_overflow, label %oom, label %align_ok\n";
        out += "align_ok:\n";
        out += "  %alloc_bytes = and i64 %align_sum, -8\n";
        out += "  %has_current = icmp ne ptr %current, null\n";
        out += "  br i1 %has_current, label %check_space, label %new_chunk\n";
        out += "check_space:\n";
        out += "  %fits = icmp ule i64 %alloc_bytes, %remaining\n";
        out += "  br i1 %fits, label %bump, label %new_chunk\n";
        out += "bump:\n";
        out += "  %new_current = getelementptr i8, ptr %current, i64 %alloc_bytes\n";
        out += "  %new_remaining = sub i64 %remaining, %alloc_bytes\n";
        out += "  store ptr %new_current, ptr %cur.addr\n";
        out += "  store i64 %new_remaining, ptr %rem.addr\n";
        out += "  ret ptr %current\n";
        out += "new_chunk:\n";
        out += "  %chunk_size = load i64, ptr %chunk.addr\n";
        out += "  %chunk_zero = icmp eq i64 %chunk_size, 0\n";
        out += "  %base_capacity = select i1 %chunk_zero, i64 %alloc_bytes, i64 %chunk_size\n";
        out += "  %double_candidate = add i64 %base_capacity, %base_capacity\n";
        out += "  %double_wrapped = icmp ult i64 %double_candidate, %base_capacity\n";
        out += "  %grown_capacity = select i1 %double_wrapped, i64 %base_capacity, i64 %double_candidate\n";
        out += "  %need_bytes = icmp ult i64 %grown_capacity, %alloc_bytes\n";
        out += "  %capacity = select i1 %need_bytes, i64 %alloc_bytes, i64 %grown_capacity\n";
        out += "  %total = add i64 %capacity, 8\n";
        out += "  %total_overflow = icmp ult i64 %total, %capacity\n";
        out += "  br i1 %total_overflow, label %oom, label %allocate\n";
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
        out += "  br label %bump_after_chunk\n";
        out += "bump_after_chunk:\n";
        out += "  %fresh_current = load ptr, ptr %cur.addr\n";
        out += "  %fresh_remaining = load i64, ptr %rem.addr\n";
        out += "  %next_current = getelementptr i8, ptr %fresh_current, i64 %alloc_bytes\n";
        out += "  %next_remaining = sub i64 %fresh_remaining, %alloc_bytes\n";
        out += "  store ptr %next_current, ptr %cur.addr\n";
        out += "  store i64 %next_remaining, ptr %rem.addr\n";
        out += "  ret ptr %fresh_current\n";
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
        out += "  %head.addr2 = getelementptr inbounds %LannerArena, ptr %arena, i32 0, i32 0\n";
        out += "  %cur.addr2 = getelementptr inbounds %LannerArena, ptr %arena, i32 0, i32 1\n";
        out += "  %rem.addr2 = getelementptr inbounds %LannerArena, ptr %arena, i32 0, i32 2\n";
        out += "  store ptr null, ptr %head.addr2\n";
        out += "  store ptr null, ptr %cur.addr2\n";
        out += "  store i64 0, ptr %rem.addr2\n";
        out += "  ret void\n";
        out += "}\n\n";
    }

    for (const auto& decl : program.decls) {
        if (decl->kind != DeclKind::Struct) continue;
        out += "%" + decl->st->name + " = type " + std::string(decl->st->isPacked ? "<{ " : "{ ");
        for (std::size_t i = 0; i < decl->st->fields.size(); ++i) {
            if (i) out += ", ";
            out += llvmType(decl->st->fields[i].type.get());
        }
        out += decl->st->isPacked ? " }>\n" : " }\n";
    }
    if (!structs.empty()) out += "\n";
    for (const auto& decl : program.decls) {
        if (decl->kind == DeclKind::Static) emitStaticDecl(*decl->staticDecl, out);
    }
    if (!statics.empty()) out += "\n";

    for (const auto& decl : program.decls) {
        if (decl->kind == DeclKind::Function) emitFunction(*decl->fn, out);
    }
    if (includeRuntime && !isWebTarget()) {
        auto it = functions.find("main");
        if (it != functions.end() && !it->second->isExtern) {
            const auto retType = llvmType(it->second->returnType.get());
            out += "define i32 @main(i32 %argc, ptr %argv) {\n";
            out += "entry:\n";
            out += "  call void @__lanner_process_set_argv(i32 %argc, ptr %argv)\n";
            if (retType == "i32") {
                out += "  %lanner.main.ret = call i32 @lanner_user_main()\n";
                out += "  ret i32 %lanner.main.ret\n";
            } else {
                out += "  call " + retType + " @lanner_user_main()\n";
                out += "  ret i32 0\n";
            }
            out += "}\n\n";
        }
    }
    for (const auto& decl : program.decls) {
        if (decl->kind == DeclKind::Function) emitThreadWrapper(*decl->fn, out);
    }
    if (!stringLiterals.empty()) {
        out += "; string literals\n";
        for (const auto& global : stringLiterals) out += global + "\n";
        out += "\n";
    }
    return out;
}
