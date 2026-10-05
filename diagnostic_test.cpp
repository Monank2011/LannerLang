#include "src/diagnostics/diagnostic.hpp"
#include <iostream>
#include <string>

int main() {
    const std::string source =
        "fn add(a: i32, b: i32) i32:\n"
        "    return a + b\n";

    const auto parsed = stable::diagnostics::parseErrorMessage(
        "Type error at 2:12: arithmetic operands must have the same numeric type");
    if (parsed.phase != "type checking" || parsed.line != 2 || parsed.column != 12) return 1;
    if (parsed.hint.empty()) return 2;

    const std::string rendered = stable::diagnostics::render(parsed, "demo.st", source);
    if (rendered.find("demo.st:2:12: error:") == std::string::npos) return 3;
    if (rendered.find("2 |     return a + b") == std::string::npos) return 4;
    if (rendered.find("= help:") == std::string::npos) return 5;

    const auto parseOnly = stable::diagnostics::parseErrorMessage(
        "Parse error at 7:5: expected ':' after if condition");
    if (parseOnly.line != 7 || parseOnly.column != 5 || parseOnly.hint.empty()) return 6;

    std::cout << "diagnostic_test: PASS\n";
    return 0;
}
