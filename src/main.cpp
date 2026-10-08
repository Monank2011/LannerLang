#include "driver/compiler_driver.hpp"
#include "codegen/llvm_codegen.hpp"
#include "diagnostics/diagnostic.hpp"
#if LANNER_ENABLE_LEGACY_HIR
#include "ir/hir.hpp"
#include "ir/hir_lowerer.hpp"
#include "ir/hir_llvm_codegen.hpp"
#include "ir/hir_optimizer.hpp"
#endif
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "sema/symbol_table.hpp"
#include "sema/typechecker.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

static void usage() {
    std::cerr
        << "Lanner compiler\n"
        << "usage: lanner <file.lan> [options]\n\n"
        << "build/run:\n"
        << "  lanner file.lan                 build executable (default)\n"
        << "  lanner file.lan -o app          choose executable path\n"
        << "  lanner file.lan --run            build temporarily and launch it\n"
        << "  lanner file.lan --script         compile and execute as a script; use -- to pass script arguments\n"
        << "  lanner file.lan --check          type-check only\n"
        << "  lanner file.lan --emit-llvm      print LLVM IR\n"
        << "  lanner file.lan --emit-llvm -o x.ll\n"
        << "  lanner file.lan --emit-object -o x.o\n"
        << "  lanner file.lan --emit-asm -o x.s\n"
        << "\nsystems/targets:\n"
        << "  --target=<triple>               LLVM target triple for native/cross builds\n"
        << "  --sysroot <path>                target sysroot/root for toolchain and linker\n"
        << "  --cpu <name>                    target CPU or architecture tuning\n"
        << "  --native                         optimize for the build machine (implies native CPU tuning)\n"
        << "  --keep-symbols                  keep external linkage for all functions (disables whole-program internalization)\n"
        << "  --features <f1,f2,...>          target feature flags\n"
        << "  --linker <name>                 select linker (e.g. lld, mold)\n"
        << "  --linker-script <path>          pass a custom linker script\n"
        << "  --freestanding                  no Lanner runtime, no hosted startup/libs\n"
        << "  --no-runtime                    omit the Lanner hosted runtime only\n"
        << "  --entry <symbol>                entry symbol for freestanding links\n"
        << "  --web                           build a browser-ready WebAssembly module + JS loader\n"
        << "  --android-project <dir>         generate an Android Studio + NDK project\n"
        << "  --ios-project <dir>             generate an Xcode + SwiftUI project\n"
        << "  --android-abi <abi>             arm64-v8a, armeabi-v7a, x86_64, or x86\n"
        << "  --ios-simulator                 target arm64 iOS Simulator when generating an iOS project\n"
        << "  --mobile-name <name>            generated app name\n"
        << "  --bundle-id <id>                generated Android/iOS bundle identifier\n"
        << "  --deployment <version>          mobile deployment/API version\n"
        << "  --game-project <dir>            generate a cross-platform native game project\n"
        << "  --game-name <name>              generated game executable/project name\n"
        << "  --game-backend <name>           game platform layer (currently sdl2)\n"
        << "\noptimization:\n"
        << "  -O0 -O1 -O2 -O3                 LLVM optimization level (default O2)\n"
        << "  --link <arg>                     pass a library/object/linker argument to final clang link\n"
        << "\nIR inspection:\n"
        << "  --emit-hir --emit-llvm-hir\n"
        << "  --emit-opt-hir --emit-opt-llvm-hir\n"
        << "\nbackend:\n"
        << "  --backend=auto                 native LLVM backend (default)\n"
        << "  --backend=hir                  legacy bootstrap backend (if enabled)\n"
        << "  --backend=llvm                 use legacy direct LLVM backend\n";
}

static bool looksLikeMode(const std::string& arg) {
    return arg == "--check" || arg == "--emit-llvm" || arg == "--emit-object" ||
           arg == "--emit-asm" || arg == "--run" || arg == "--script" || arg == "--emit-hir" ||
           arg == "--emit-llvm-hir" || arg == "--emit-opt-hir" || arg == "--emit-opt-llvm-hir";
}

static bool parseBackend(const std::string& arg, BackendKind& backend) {
    constexpr const char* prefix = "--backend=";
    if (arg.rfind(prefix, 0) != 0) return false;
    const std::string value = arg.substr(10);
    if (value == "auto") backend = BackendKind::Auto;
    else if (value == "hir") backend = BackendKind::HIR;
    else if (value == "llvm") backend = BackendKind::LLVM;
    else return false;
    return true;
}

static bool parseOptimization(const std::string& arg, int& level) {
    if (arg.size() != 3 || arg[0] != '-' || arg[1] != 'O' || arg[2] < '0' || arg[2] > '3') return false;
    level = arg[2] - '0';
    return true;
}

static std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open '" + path + "'");
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        usage();
        return 2;
    }

    if (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
        usage();
        return 0;
    }
    if (std::string(argv[1]) == "--version") {
        std::cout << "Lanner 3.0.0 (native LLVM production backend; C++ bootstrap frontend)\n";
        return 0;
    }

    CompilerOptions options;
    options.inputPath = argv[1];
    std::string modeName = "build";
    bool explicitTarget = false;

    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--") {
            for (++i; i < argc; ++i) options.runArgs.emplace_back(argv[i]);
            break;
        }
        if (arg == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "lanner: -o requires a path\n";
                return 2;
            }
            options.outputPath = argv[++i];
            continue;
        }
        if (parseOptimization(arg, options.optimizationLevel)) continue;
        if (arg == "--android-project") {
            if (i + 1 >= argc) { std::cerr << "lanner: --android-project requires a directory\n"; return 2; }
            options.mobilePlatform = "android"; options.mobileProjectPath = argv[++i]; options.mode = BuildMode::GenerateAndroidProject;
            options.noRuntime = true; if (options.targetTriple.empty()) options.targetTriple = "aarch64-linux-android21"; continue;
        }
        if (arg == "--ios-project") {
            if (i + 1 >= argc) { std::cerr << "lanner: --ios-project requires a directory\n"; return 2; }
            options.mobilePlatform = "ios"; options.mobileProjectPath = argv[++i]; options.mode = BuildMode::GenerateIOSProject;
            options.noRuntime = true; if (options.targetTriple.empty()) options.targetTriple = "arm64-apple-ios16.0"; continue;
        }
        if (arg == "--game-project") {
            if (i + 1 >= argc) { std::cerr << "lanner: --game-project requires a directory\n"; return 2; }
            options.gameProjectPath = argv[++i]; options.mode = BuildMode::GenerateGameProject; continue;
        }
        if (arg == "--game-name") {
            if (i + 1 >= argc) { std::cerr << "lanner: --game-name requires a name\n"; return 2; }
            options.gameAppName = argv[++i]; continue;
        }
        if (arg == "--game-backend") {
            if (i + 1 >= argc) { std::cerr << "lanner: --game-backend requires a backend name\n"; return 2; }
            options.gameBackend = argv[++i]; if (options.gameBackend != "sdl2") { std::cerr << "lanner: unsupported game backend '" << options.gameBackend << "' (currently supported: sdl2)\n"; return 2; } continue;
        }
        if (arg == "--android-abi") {
            if (i + 1 >= argc) { std::cerr << "lanner: --android-abi requires an ABI\n"; return 2; }
            options.mobilePlatform = "android"; options.mobileAbi = argv[++i]; options.noRuntime = true; continue;
        }
        if (arg == "--ios-simulator") {
            options.mobilePlatform = "ios"; options.iosSimulator = true; options.noRuntime = true;
            if (options.targetTriple.empty()) options.targetTriple = "arm64-apple-ios16.0-simulator";
            continue;
        }
        if (arg == "--mobile-name") {
            if (i + 1 >= argc) { std::cerr << "lanner: --mobile-name requires a name\n"; return 2; }
            options.mobileAppName = argv[++i]; continue;
        }
        if (arg == "--bundle-id") {
            if (i + 1 >= argc) { std::cerr << "lanner: --bundle-id requires an identifier\n"; return 2; }
            options.mobileBundleId = argv[++i]; continue;
        }
        if (arg == "--deployment") {
            if (i + 1 >= argc) { std::cerr << "lanner: --deployment requires a version\n"; return 2; }
            options.mobileDeployment = argv[++i]; continue;
        }
        if (arg == "--freestanding") { options.freestanding = true; options.noRuntime = true; continue; }
        if (arg == "--web") {
            options.web = true;
            options.freestanding = true;
            options.noRuntime = true;
            if (options.targetTriple.empty()) options.targetTriple = "wasm32-unknown-unknown";
            continue;
        }
        if (arg == "--no-runtime") { options.noRuntime = true; continue; }
        if (arg.rfind("--target=", 0) == 0) { options.targetTriple = arg.substr(9); explicitTarget = true; if (options.targetTriple.empty()) { std::cerr << "lanner: --target requires a triple\n"; return 2; } continue; }
        if (arg == "--target") { if (i + 1 >= argc) { std::cerr << "lanner: --target requires a triple\n"; return 2; } options.targetTriple = argv[++i]; explicitTarget = true; continue; }
        if (arg == "--sysroot") { if (i + 1 >= argc) { std::cerr << "lanner: --sysroot requires a path\n"; return 2; } options.sysroot = argv[++i]; continue; }
        if (arg == "--linker") { if (i + 1 >= argc) { std::cerr << "lanner: --linker requires a linker name\n"; return 2; } options.linker = argv[++i]; continue; }
        if (arg == "--linker-script") { if (i + 1 >= argc) { std::cerr << "lanner: --linker-script requires a path\n"; return 2; } options.linkerScript = argv[++i]; continue; }
        if (arg == "--native") { options.native = true; continue; }
        if (arg == "--keep-symbols") { options.keepSymbols = true; continue; }
        if (arg == "--cpu") { if (i + 1 >= argc) { std::cerr << "lanner: --cpu requires a CPU name\n"; return 2; } options.cpu = argv[++i]; continue; }
        if (arg == "--features") { if (i + 1 >= argc) { std::cerr << "lanner: --features requires a comma-separated feature list\n"; return 2; } options.features = argv[++i]; continue; }
        if (arg == "--entry") { if (i + 1 >= argc) { std::cerr << "lanner: --entry requires a symbol\n"; return 2; } options.entryPoint = argv[++i]; continue; }
        if (arg == "--link") {
            if (i + 1 >= argc) {
                std::cerr << "lanner: --link requires an argument\n";
                return 2;
            }
            options.linkArgs.push_back(argv[++i]);
            continue;
        }
        if (parseBackend(arg, options.backend)) continue;
        if (looksLikeMode(arg)) {
            if (!modeName.empty() && modeName != "build") {
                std::cerr << "lanner: choose only one compilation mode\n";
                return 2;
            }
            modeName = arg;
            continue;
        }
        std::cerr << "lanner: unknown option '" << arg << "'\n";
        usage();
        return 2;
    }

    if (!explicitTarget && options.mobilePlatform == "android") {
        if (options.mobileAbi == "armeabi-v7a" || options.mobileAbi == "armv7") options.targetTriple = "armv7a-linux-androideabi" + options.mobileDeployment;
        else if (options.mobileAbi == "x86_64") options.targetTriple = "x86_64-linux-android" + options.mobileDeployment;
        else if (options.mobileAbi == "x86") options.targetTriple = "i686-linux-android" + options.mobileDeployment;
        else options.targetTriple = "aarch64-linux-android" + options.mobileDeployment;
    } else if (!explicitTarget && options.mobilePlatform == "ios") {
        options.targetTriple = "arm64-apple-ios" + options.mobileDeployment + (options.iosSimulator ? "-simulator" : "");
    }

    if (options.mode == BuildMode::GenerateAndroidProject || options.mode == BuildMode::GenerateIOSProject || options.mode == BuildMode::GenerateGameProject) {
        return CompilerDriver{}.run(options);
    }
    if (modeName == "--check" || modeName == "--run" || modeName == "--script" || modeName == "build" ||
        modeName == "--emit-llvm" || modeName == "--emit-object" || modeName == "--emit-asm") {
        if (modeName == "--check") options.mode = BuildMode::Check;
        else if (modeName == "--run" || modeName == "--script") options.mode = BuildMode::Run;
        else if (modeName == "--emit-llvm") options.mode = BuildMode::EmitLLVM;
        else if (modeName == "--emit-object") options.mode = BuildMode::EmitObject;
        else if (modeName == "--emit-asm") options.mode = BuildMode::EmitAssembly;
        else options.mode = BuildMode::BuildExecutable;
        return CompilerDriver{}.run(options);
    }

    try {
        const std::string source = readFile(options.inputPath);
        Lexer lexer(source);
        Parser parser(lexer.tokenize());
        Program program = parser.parseProgram();

        SymbolTable symbols;
        TypeChecker checker(symbols);
        checker.checkProgram(program);

#if LANNER_ENABLE_LEGACY_HIR
        lanner::hir::Lowerer lowerer;
        lanner::hir::Module module = lowerer.lowerProgram(program);
        const bool optimized = modeName == "--emit-opt-hir" || modeName == "--emit-opt-llvm-hir";
        if (optimized) lanner::hir::optimize(module, lanner::hir::OptimizationLevel::O2);

        if (modeName == "--emit-hir" || modeName == "--emit-opt-hir") {
            std::cout << lanner::hir::print(module);
        } else {
            lanner::hir::LLVMCodegen codegen;
            std::cout << codegen.generate(module);
        }
        return 0;
#else
        if (modeName == "--emit-hir" || modeName == "--emit-opt-hir" || modeName == "--emit-llvm-hir" || modeName == "--emit-opt-llvm-hir") {
            std::cerr << "legacy HIR inspection is disabled in the production compiler; rebuild with LANNER_ENABLE_LEGACY_HIR=ON" << std::endl;
            return 2;
        }
        LLVMCodeGenerator codegen;
        std::cout << codegen.generate(program);
        return 0;
#endif
    } catch (const std::exception& e) {
        const std::string source = [&]() {
            try { return readFile(options.inputPath); }
            catch (...) { return std::string{}; }
        }();
        const auto parsed = lanner::diagnostics::parseErrorMessage(e.what());
        std::cerr << lanner::diagnostics::render(parsed, options.inputPath, source);
        return 1;
    }
}
