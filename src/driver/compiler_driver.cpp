#include "compiler_driver.hpp"
#include "../codegen/llvm_codegen.hpp"
#if STABLE_ENABLE_LEGACY_HIR
#include "../ir/hir.hpp"
#include "../ir/hir_lowerer.hpp"
#include "../ir/hir_llvm_codegen.hpp"
#include "../ir/hir_optimizer.hpp"
#endif
#include "../diagnostics/diagnostic.hpp"
#include "../lexer/lexer.hpp"
#include "../parser/parser.hpp"
#include "../sema/symbol_table.hpp"
#include "../sema/typechecker.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <regex>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#include <sys/wait.h>
#endif

namespace {

std::string removeExtension(const std::string& path) {
    const std::filesystem::path p(path);
    return (p.parent_path() / p.stem()).string();
}

std::string clangExecutable() {
    if (const char* value = std::getenv("STABLE_CLANG")) {
        if (*value != '\0') return value;
    }
    if (const char* value = std::getenv("LLVM_CC")) {
        if (*value != '\0') return value;
    }
#ifdef STABLE_CLANG_EXECUTABLE
    return STABLE_CLANG_EXECUTABLE;
#else
    return "clang";
#endif
}

std::string captureCommand(const std::string& command) {
#if defined(__unix__) || defined(__APPLE__)
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return {};
    std::string output;
    char buffer[256];
    while (std::fgets(buffer, sizeof(buffer), pipe)) output += buffer;
    (void)pclose(pipe);
    return output;
#elif defined(_WIN32)
    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) return {};
    std::string output;
    char buffer[256];
    while (std::fgets(buffer, sizeof(buffer), pipe)) output += buffer;
    (void)_pclose(pipe);
    return output;
#else
    (void)command;
    return {};
#endif
}

std::string clangTargetTriple(const std::string& executable) {
    std::string quoted = "'";
    for (const char c : executable) {
        if (c == '\'') quoted += "'\\''";
        else quoted += c;
    }
    quoted += "'";
    std::string target = captureCommand(quoted + " --print-target-triple 2>/dev/null");
    target.erase(std::remove(target.begin(), target.end(), '\n'), target.end());
    target.erase(std::remove(target.begin(), target.end(), '\r'), target.end());
    return target;
}

std::string webJSPathFor(const std::string& wasmPath) {
    std::filesystem::path p(wasmPath);
    p.replace_extension(".js");
    return p.string();
}

std::string webLoaderSource(const std::string& wasmFileName) {
    std::string out = R"JS(const utf8 = new TextDecoder("utf-8");
const utf8Encoder = new TextEncoder();

export async function loadStable(url = new URL("WASM_FILE", import.meta.url), extraImports = {}) {
    let instance = null;
    let memory = null;
    let heap = 0;
    const allocations = new Map();
    const timers = new Map();
    const listeners = new Map();
    let nextListenerId = 1;

    const bytes = () => new Uint8Array(memory.buffer);
    const cstr = (ptr) => {
        if (!ptr) return "";
        const data = bytes();
        let end = ptr;
        const limit = Math.min(data.length, ptr + (16 * 1024 * 1024));
        while (end < limit && data[end] !== 0) end++;
        return utf8.decode(data.subarray(ptr, end));
    };
    const alignUp = (n, a) => (n + (a - 1)) & ~(a - 1);
    const alloc = (size, alignment = 8) => {
        size = Math.max(1, Number(size) >>> 0);
        alignment = Math.max(1, Number(alignment) >>> 0);
        heap = alignUp(heap, alignment);
        const ptr = heap;
        const end = ptr + size;
        const neededPages = Math.ceil(end / 65536);
        if (neededPages > memory.buffer.byteLength / 65536) {
            memory.grow(neededPages - memory.buffer.byteLength / 65536);
        }
        heap = end;
        allocations.set(ptr, size);
        return ptr;
    };
    const free = (ptr) => { allocations.delete(Number(ptr) >>> 0); };
    const copyCString = (text) => {
        const data = utf8Encoder.encode(String(text));
        const ptr = alloc(data.length + 1, 1);
        bytes().set(data, ptr);
        bytes()[ptr + data.length] = 0;
        return ptr;
    };
    const importFn = (name, fallback) => extraImports[name] ?? fallback;

    const env = {
        ...extraImports,
        malloc: importFn("malloc", alloc),
        realloc: importFn("realloc", (ptr, size) => {
            const old = allocations.get(Number(ptr) >>> 0) ?? 0;
            const next = alloc(size, 8);
            const source = Number(ptr) >>> 0;
            bytes().copyWithin(next, source, source + Math.min(old, Number(size) >>> 0));
            free(ptr);
            return next;
        }),
        free: importFn("free", free),
        memset: importFn("memset", (ptr, value, size) => {
            bytes().fill(Number(value) & 255, Number(ptr) >>> 0, (Number(ptr) + Number(size)) >>> 0);
            return Number(ptr) >>> 0;
        }),
        memcpy: importFn("memcpy", (dst, src, size) => {
            const d = Number(dst) >>> 0, s = Number(src) >>> 0, n = Number(size) >>> 0;
            bytes().copyWithin(d, s, s + n); return d;
        }),
        memmove: importFn("memmove", (dst, src, size) => {
            const d = Number(dst) >>> 0, s = Number(src) >>> 0, n = Number(size) >>> 0;
            const tmp = bytes().slice(s, s + n); bytes().set(tmp, d); return d;
        }),
        memcmp: importFn("memcmp", (a, b, size) => {
            const aa = bytes(), x = Number(a) >>> 0, y = Number(b) >>> 0, n = Number(size) >>> 0;
            for (let i = 0; i < n; ++i) { if (aa[x+i] !== aa[y+i]) return aa[x+i] - aa[y+i]; } return 0;
        }),
        strlen: importFn("strlen", (ptr) => cstr(ptr).length),
        __stable_web_log: importFn("__stable_web_log", (ptr) => console.log(cstr(ptr))),
        __stable_web_warn: importFn("__stable_web_warn", (ptr) => console.warn(cstr(ptr))),
        __stable_web_error: importFn("__stable_web_error", (ptr) => console.error(cstr(ptr))),
        __stable_web_now_ms: importFn("__stable_web_now_ms", () => performance.now()),
        __stable_web_random: importFn("__stable_web_random", () => Math.random()),
        __stable_web_set_text: importFn("__stable_web_set_text", (selector, text) => { const e=document.querySelector(cstr(selector)); if(!e)return 0; e.textContent=cstr(text); return 1; }),
        __stable_web_set_html: importFn("__stable_web_set_html", (selector, html) => { const e=document.querySelector(cstr(selector)); if(!e)return 0; e.innerHTML=cstr(html); return 1; }),
        __stable_web_set_attr: importFn("__stable_web_set_attr", (selector, name, value) => { const e=document.querySelector(cstr(selector)); if(!e)return 0; e.setAttribute(cstr(name), cstr(value)); return 1; }),
        __stable_web_add_class: importFn("__stable_web_add_class", (selector, cls) => { const e=document.querySelector(cstr(selector)); if(!e)return 0; e.classList.add(cstr(cls)); return 1; }),
        __stable_web_remove_class: importFn("__stable_web_remove_class", (selector, cls) => { const e=document.querySelector(cstr(selector)); if(!e)return 0; e.classList.remove(cstr(cls)); return 1; }),
        __stable_web_remove: importFn("__stable_web_remove", (selector) => { const e=document.querySelector(cstr(selector)); if(!e)return 0; e.remove(); return 1; }),
        __stable_web_query_count: importFn("__stable_web_query_count", (selector) => document.querySelectorAll(cstr(selector)).length),
        __stable_web_focus: importFn("__stable_web_focus", (selector) => { const e=document.querySelector(cstr(selector)); if(!e)return 0; e.focus(); return 1; }),
        __stable_web_set_timeout: importFn("__stable_web_set_timeout", (callbackName, ms) => { const id = setTimeout(() => { const f=instance?.exports?.[cstr(callbackName)]; if(typeof f === "function") f(); }, Number(ms)); timers.set(id,id); return id|0; }),
        __stable_web_clear_timeout: importFn("__stable_web_clear_timeout", (id) => { clearTimeout(Number(id)); timers.delete(Number(id)); }),
        __stable_web_request_animation_frame: importFn("__stable_web_request_animation_frame", (callbackName) => { const id=requestAnimationFrame((t)=>{ const f=instance?.exports?.[cstr(callbackName)]; if(typeof f === "function") f(t); }); timers.set(id,id); return id|0; }),
        __stable_web_cancel_animation_frame: importFn("__stable_web_cancel_animation_frame", (id) => { cancelAnimationFrame(Number(id)); timers.delete(Number(id)); }),
        __stable_web_queue_microtask: importFn("__stable_web_queue_microtask", (callbackName) => { queueMicrotask(()=>{ const f=instance?.exports?.[cstr(callbackName)]; if(typeof f === "function") f(); }); return 0; }),
        __stable_web_add_event_listener: importFn("__stable_web_add_event_listener", (selector, event, callbackName) => {
            const e=document.querySelector(cstr(selector)); if(!e)return 0; const name=cstr(callbackName); const fn=()=>{ const f=instance?.exports?.[name]; if(typeof f === "function") f(); }; e.addEventListener(cstr(event),fn); const id=nextListenerId++; listeners.set(id,{e,event:cstr(event),fn}); return id;
        }),
        __stable_web_remove_event_listener: importFn("__stable_web_remove_event_listener", (id) => { const r=listeners.get(Number(id)); if(!r)return 0; r.e.removeEventListener(r.event,r.fn); listeners.delete(Number(id)); return 1; }),
        __stable_web_fetch_text: importFn("__stable_web_fetch_text", (urlPtr, callbackPtr) => {
            const urlText=cstr(urlPtr), cbName=cstr(callbackPtr);
            fetch(urlText).then(async r => {
                const buf=new Uint8Array(await r.arrayBuffer());
                const ptr=alloc(buf.length+1,1); bytes().set(buf,ptr); bytes()[ptr+buf.length]=0;
                const f=instance?.exports?.[cbName]; if(typeof f === "function") f(r.status,ptr,buf.length);
            }).catch(() => {
                const f=instance?.exports?.[cbName]; if(typeof f === "function") f(0,0,0);
            });
            return 0;
        }),
        __stable_web_buffer_free: importFn("__stable_web_buffer_free", free),
    };

    const imports = { env };
    const response = await fetch(url);
    let result;
    try { result = await WebAssembly.instantiateStreaming(response, imports); }
    catch (_) { const bytes = await (await fetch(url)).arrayBuffer(); result = await WebAssembly.instantiate(bytes, imports); }
    instance = result.instance ?? result;
    memory = instance.exports.memory;
    if (!memory) throw new Error("Stable Web module did not export linear memory");
    const hb = instance.exports.__heap_base;
    heap = hb && "value" in hb ? Number(hb.value) : 1024;
    return { instance, module: result.module ?? null, memory, exports: instance.exports };
}

export const ready = loadStable;
)JS";
    const std::string marker = "WASM_FILE";
    std::size_t pos = 0;
    while ((pos = out.find(marker, pos)) != std::string::npos) { out.replace(pos, marker.size(), wasmFileName); pos += wasmFileName.size(); }
    return out;
}

int clangMajorVersion(const std::string& executable) {
    std::string quoted = "'";
    for (const char c : executable) {
        if (c == '\'') quoted += "'\\''";
        else quoted += c;
    }
    quoted += "'";
    const std::string command = quoted + " --version 2>/dev/null";
    const std::string text = captureCommand(command);
    std::smatch match;
    static const std::regex llvmVersion(R"((?:clang|LLVM) version ([0-9]+))", std::regex::icase);
    if (std::regex_search(text, match, llvmVersion) && match.size() > 1) {
        try { return std::stoi(match[1].str()); }
        catch (...) { return 0; }
    }
    return 0;
}

}

std::string stableRuntimePath() {
    if (const char* value = std::getenv("STABLE_RUNTIME")) return value;
#ifdef STABLE_RUNTIME_SOURCE
    return STABLE_RUNTIME_SOURCE;
#else
    return {};
#endif
}

std::string CompilerDriver::readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open '" + path + "'");
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::string CompilerDriver::quoteShellArg(const std::string& value) {
#if defined(_WIN32)
    // cmd.exe/MSVCRT-compatible quoting. Escape embedded quotes and preserve
    // trailing backslashes so Windows paths remain one argument.
    std::string quoted = "\"";
    std::size_t backslashes = 0;
    for (const char c : value) {
        if (c == '\\') {
            ++backslashes;
            continue;
        }
        if (c == '\"') {
            quoted.append(backslashes * 2 + 1, '\\');
            quoted += '\"';
            backslashes = 0;
            continue;
        }
        quoted.append(backslashes, '\\');
        backslashes = 0;
        quoted += c;
    }
    quoted.append(backslashes * 2, '\\');
    quoted += '\"';
    return quoted;
#else
    // POSIX shell single-quoted argument escaping.
    std::string quoted = "'";
    for (const char c : value) {
        if (c == '\'') quoted += "'\\''";
        else quoted += c;
    }
    quoted += "'";
    return quoted;
#endif
}

std::string CompilerDriver::defaultOutputPath(const std::string& inputPath) {
    return removeExtension(inputPath);
}

std::string CompilerDriver::makeTemporaryPath(const std::string& inputPath) {
    const auto base = std::filesystem::temp_directory_path();
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    long long pid = 0;
#if defined(__unix__) || defined(__APPLE__)
    pid = static_cast<long long>(::getpid());
#endif
    return (base / (std::filesystem::path(inputPath).stem().string() + "." +
                    std::to_string(pid) + "." + std::to_string(now) + ".stable.ll")).string();
}

bool CompilerDriver::hasMain(const Program& program) {
    for (const auto& decl : program.decls) {
        if (decl->kind == DeclKind::Function && decl->fn && decl->fn->name == "main") return true;
    }
    return false;
}

std::string CompilerDriver::generateLLVM(const Program& program, BackendKind backend, int optimizationLevel, BackendKind& usedBackend, bool includeRuntime, const std::string& targetTriple) {
#if !STABLE_ENABLE_LEGACY_HIR
    (void)optimizationLevel;
    (void)includeRuntime;
    (void)targetTriple;
#endif
    // Native LLVM is now the production backend.  HIR remains only as an
    // explicitly gated bootstrap/research path while the Stable-written
    // compiler is brought to feature parity.  Production `auto` never
    // silently falls back to HIR.
    if (backend == BackendKind::HIR) {
#if STABLE_ENABLE_LEGACY_HIR
        stable::hir::Lowerer lowerer;
        stable::hir::Module module = lowerer.lowerProgram(program);
        if (optimizationLevel > 0) {
            const auto level = optimizationLevel >= 2 ? stable::hir::OptimizationLevel::O2
                                                      : stable::hir::OptimizationLevel::O1;
            stable::hir::optimize(module, level);
        }
        stable::hir::LLVMCodegen codegen;
        usedBackend = BackendKind::HIR;
        return codegen.generate(module);
#else
        throw std::runtime_error("legacy HIR backend is disabled; use --backend=llvm");
#endif
    }

    LLVMCodeGenerator codegen;
    usedBackend = BackendKind::LLVM;
    return codegen.generate(program, includeRuntime, targetTriple);
}

bool CompilerDriver::writeTextFile(const std::string& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out << text;
    return static_cast<bool>(out);
}

int CompilerDriver::invokeClang(const std::vector<std::string>& args) {
    std::string command = quoteShellArg(clangExecutable());
    for (const auto& arg : args) {
        command.push_back(' ');
        command += quoteShellArg(arg);
    }
    const int status = std::system(command.c_str());
    if (status == -1) return -1;
#if defined(__unix__) || defined(__APPLE__)
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
#endif
    return status;
}

int CompilerDriver::run(const CompilerOptions& options) {
    std::string source;

    try {
        if (options.web && (options.targetTriple.empty() || options.targetTriple.rfind("wasm32", 0) != 0)) {
            throw std::runtime_error("--web requires a wasm32 target triple");
        }
        if (options.web && (options.mode == BuildMode::EmitObject || options.mode == BuildMode::EmitAssembly)) {
            throw std::runtime_error("--web produces a WebAssembly module; do not combine it with --emit-object or --emit-asm");
        }
        source = readFile(options.inputPath);
        Lexer lexer(source);
        Parser parser(lexer.tokenize());
        Program program = parser.parseProgram();

        SymbolTable symbols;
        TypeChecker checker(symbols);
        checker.setTargetTriple(options.targetTriple);
        checker.checkProgram(program);

        if (options.mode == BuildMode::GenerateAndroidProject) return generateAndroidProject(options, source);
        if (options.mode == BuildMode::GenerateIOSProject) return generateIOSProject(options, source);
        if (options.mode == BuildMode::GenerateGameProject) return generateGameProject(options, source);

        if (options.mode == BuildMode::Check) {
            std::cout << "OK: " << options.inputPath << "\n";
            return 0;
        }

        const bool executableMode = options.mode == BuildMode::BuildExecutable || options.mode == BuildMode::Run;
        const bool freestanding = options.freestanding;
        if (executableMode) {
            if (freestanding && !options.web) {
                if (options.entryPoint.empty()) {
                    throw std::runtime_error("freestanding executable builds require --entry <symbol>");
                }
                bool foundEntry = false;
                for (const auto& decl : program.decls) {
                    if (decl->kind == DeclKind::Function && decl->fn && decl->fn->name == options.entryPoint && !decl->fn->isExtern) {
                        foundEntry = true; break;
                    }
                }
                if (!foundEntry) throw std::runtime_error("freestanding entry function '" + options.entryPoint + "' is not defined in the program");
            } else if (!freestanding && !hasMain(program)) {
                throw std::runtime_error("Type error at 1:1: executable builds require a 'main' function");
            }
        }

        BackendKind usedBackend = BackendKind::Auto;
        const std::string llvm = generateLLVM(program, options.backend, options.optimizationLevel, usedBackend, !options.noRuntime, options.targetTriple);

        if (options.mode == BuildMode::EmitLLVM) {
            const std::string output = options.outputPath;
            if (output.empty()) {
                std::cout << llvm;
                return 0;
            }
            if (!std::filesystem::path(output).parent_path().empty()) {
                std::error_code mkdirEc;
                std::filesystem::create_directories(std::filesystem::path(output).parent_path(), mkdirEc);
                if (mkdirEc) throw std::runtime_error("driver error: cannot create output directory '" +
                                                       std::filesystem::path(output).parent_path().string() + "'");
            }
            if (!writeTextFile(output, llvm)) {
                throw std::runtime_error("driver error: cannot write LLVM IR to '" + output + "'");
            }
            std::cout << "wrote " << output << "\n";
            return 0;
        }

        const bool compileOnly = options.mode == BuildMode::EmitObject || options.mode == BuildMode::EmitAssembly;
        const bool temporaryRunOutput = options.mode == BuildMode::Run && options.outputPath.empty();
        const std::string llPath = makeTemporaryPath(options.inputPath);
        std::string output;
        if (temporaryRunOutput) {
            output = (std::filesystem::path(llPath).replace_extension(
#if defined(_WIN32)
                ".stable.run.exe"
#else
                ".stable.run"
#endif
            )).string();
        } else if (!options.outputPath.empty()) {
            output = options.outputPath;
        } else {
            output = defaultOutputPath(options.inputPath);
            if (options.web) output += ".wasm";
            else if (options.mode == BuildMode::EmitObject) output += ".o";
            else if (options.mode == BuildMode::EmitAssembly) output += ".s";
        }
        if (!std::filesystem::path(output).parent_path().empty()) {
            std::error_code mkdirEc;
            std::filesystem::create_directories(std::filesystem::path(output).parent_path(), mkdirEc);
            if (mkdirEc) throw std::runtime_error("driver error: cannot create output directory '" +
                                                   std::filesystem::path(output).parent_path().string() + "'");
        }
        if (!writeTextFile(llPath, llvm)) {
            throw std::runtime_error("driver error: cannot create temporary LLVM file");
        }

        const std::string compiler = clangExecutable();
        const int major = clangMajorVersion(compiler);
        if (major > 0 && major < 15) {
            throw std::runtime_error(
                "LLVM toolchain '" + compiler + "' is too old for Stable's opaque-pointer LLVM IR "
                "(LLVM 15+ is required); select a modern clang with STABLE_CLANG or LLVM_CC");
        }
        const std::string opt = "-O" + std::to_string(options.optimizationLevel);
        std::vector<std::string> command;
        if (!options.targetTriple.empty()) command.push_back("--target=" + options.targetTriple);
        if (!options.sysroot.empty()) command.push_back("--sysroot=" + options.sysroot);
        if (!options.cpu.empty()) {
            std::string triple = options.targetTriple;
            if (triple.empty()) triple = clangTargetTriple(compiler);
            std::string lower = triple;
            std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            const bool x86 = lower.rfind("x86_64", 0) == 0 || lower.rfind("amd64", 0) == 0 ||
                              lower.rfind("i386", 0) == 0 || lower.rfind("i486", 0) == 0 ||
                              lower.rfind("i586", 0) == 0 || lower.rfind("i686", 0) == 0;
            command.push_back((x86 ? "-march=" : "-mcpu=") + options.cpu);
        }
        if (!options.features.empty()) {
            std::stringstream featureStream(options.features);
            std::string feature;
            while (std::getline(featureStream, feature, ',')) {
                if (feature.empty()) continue;
                command.push_back("-Xclang");
                command.push_back("-target-feature");
                command.push_back("-Xclang");
                command.push_back(feature);
            }
        }
        if (!options.linker.empty()) command.push_back("-fuse-ld=" + options.linker);
        command.push_back("-Wno-override-module");
        command.push_back(opt);

        if (compileOnly) {
            command.push_back(options.mode == BuildMode::EmitAssembly ? "-S" : "-c");
            if (options.freestanding) command.push_back("-ffreestanding");
            command.push_back(llPath);
            command.push_back("-o");
            command.push_back(output);
        } else {
            command.push_back(llPath);
            if (!options.noRuntime) {
                const auto runtime = stableRuntimePath();
                if (!runtime.empty()) command.push_back(runtime);
            }
            if (options.web) {
                command.push_back("-ffreestanding");
                command.push_back("-nostdlib");
                command.push_back("-nodefaultlibs");
                command.push_back("-nostartfiles");
                command.push_back("-Wl,--no-entry");
                command.push_back("-Wl,--export-all");
                command.push_back("-Wl,--allow-undefined");
            } else if (options.freestanding) {
                command.push_back("-ffreestanding");
                command.push_back("-nostdlib");
                command.push_back("-nodefaultlibs");
                command.push_back("-nostartfiles");
                command.push_back("-static");
                command.push_back("-Wl,-e," + options.entryPoint);
            }
            if (!options.linkerScript.empty()) command.push_back("-Wl,-T," + options.linkerScript);
            command.push_back("-o");
            command.push_back(output);
#if !defined(_WIN32)
            if (!options.noRuntime && !options.freestanding && !stableRuntimePath().empty()) { command.push_back("-pthread"); command.push_back("-ldl"); command.push_back("-lm"); }
#else
            if (!options.noRuntime && !options.freestanding && !stableRuntimePath().empty()) command.push_back("ws2_32.lib");
#endif
            for (const auto& linkArg : options.linkArgs) command.push_back(linkArg);
        }

        const int status = invokeClang(command);
        std::error_code ec;
        std::filesystem::remove(llPath, ec);
        if (status != 0) {
            std::ostringstream message;
            if (status == -1) message << "LLVM compiler could not be launched while building '" << output << "'";
            else message << "LLVM compiler failed while building '" << output << "' (clang exit status " << status << ")";
            if (temporaryRunOutput) std::filesystem::remove(output, ec);
            throw std::runtime_error(message.str());
        }

        if (compileOnly || options.mode == BuildMode::BuildExecutable) {
            if (options.web && !compileOnly) {
                const std::string jsPath = webJSPathFor(output);
                const std::string js = webLoaderSource(std::filesystem::path(output).filename().string());
                if (!writeTextFile(jsPath, js)) throw std::runtime_error("driver error: cannot write Web loader to '" + jsPath + "'");
                std::cout << "built " << output << "\n";
                std::cout << "built " << jsPath << "\n";
            } else {
                std::cout << (options.mode == BuildMode::EmitAssembly ? "wrote " : options.mode == BuildMode::EmitObject ? "wrote " : "built ") << output << "\n";
            }
            return 0;
        }

        std::string runPath = output;
        if (std::filesystem::path(runPath).parent_path().empty()) runPath = "./" + runPath;
        std::string runCommand = quoteShellArg(runPath);
        for (const auto& arg : options.runArgs) {
            runCommand.push_back(' ');
            runCommand += quoteShellArg(arg);
        }
        const int runStatus = std::system(runCommand.c_str());
        if (temporaryRunOutput) std::filesystem::remove(output, ec);
        if (runStatus == -1) {
            throw std::runtime_error("driver error: failed to launch '" + output + "'");
        }
#if defined(__unix__) || defined(__APPLE__)
        if (WIFEXITED(runStatus)) return WEXITSTATUS(runStatus);
        if (WIFSIGNALED(runStatus)) return 128 + WTERMSIG(runStatus);
#endif
        return runStatus;
    } catch (const std::exception& e) {
        const auto parsed = stable::diagnostics::parseErrorMessage(e.what());
        std::cerr << stable::diagnostics::render(parsed, options.inputPath, source);
        return 1;
    }
}
