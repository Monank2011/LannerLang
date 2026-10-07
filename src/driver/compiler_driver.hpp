#pragma once

#include "../parser/ast.hpp"
#include <string>
#include <vector>
#include <set>

enum class BuildMode {
    Check,
    EmitLLVM,
    EmitObject,
    EmitAssembly,
    BuildExecutable,
    Run,
    GenerateAndroidProject,
    GenerateIOSProject,
    GenerateGameProject
};

enum class BackendKind {
    Auto,
    LLVM,
    HIR
};

struct CompilerOptions {
    BuildMode mode = BuildMode::BuildExecutable;
    std::string inputPath;
    std::string outputPath;
    int optimizationLevel = 2;
    BackendKind backend = BackendKind::Auto;
    std::vector<std::string> linkArgs;
    std::string targetTriple;
    std::string sysroot;
    std::string linker;
    std::string linkerScript;
    std::string cpu;
    std::string features;
    std::string entryPoint;
    std::vector<std::string> runArgs;
    bool freestanding = false;
    bool noRuntime = false;
    bool web = false;
    std::string mobilePlatform;
    std::string mobileProjectPath;
    std::string mobileAbi = "arm64-v8a";
    std::string mobileDeployment = "21";
    std::string mobileBundleId = "com.example.lannerapp";
    std::string mobileAppName = "LannerApp";
    bool iosSimulator = false;
    std::string gameProjectPath;
    std::string gameAppName = "LannerGame";
    std::string gameBackend = "sdl2";
};

class CompilerDriver {
public:
    int run(const CompilerOptions& options);

private:
    static std::string readFile(const std::string& path);
    static std::string loadModuleSource(const std::string& path);
    static std::string quoteShellArg(const std::string& value);
    static std::string defaultOutputPath(const std::string& inputPath);
    static std::string makeTemporaryPath(const std::string& inputPath);
    static bool hasMain(const Program& program);
    static std::string generateLLVM(const Program& program, BackendKind backend, int optimizationLevel, BackendKind& usedBackend, bool includeRuntime, const std::string& targetTriple);
    static int invokeClang(const std::vector<std::string>& args);
    static bool writeTextFile(const std::string& path, const std::string& text);
    static int generateAndroidProject(const CompilerOptions& options, const std::string& source);
    static int generateIOSProject(const CompilerOptions& options, const std::string& source);
    static int generateGameProject(const CompilerOptions& options, const std::string& source);
    static std::string mobileAbiTriple(const CompilerOptions& options);
    static std::string makeMobileHeader(const Program& program, const std::string& symbol = "lanner_app_main");
};
