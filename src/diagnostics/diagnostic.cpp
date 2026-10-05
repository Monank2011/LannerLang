#include "diagnostic.hpp"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string_view>

namespace stable::diagnostics {
namespace {

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string phaseFromPrefix(const std::string& prefix) {
    if (prefix.find("Lex error") != std::string::npos) return "lexing";
    if (prefix.find("Parse error") != std::string::npos) return "parsing";
    if (prefix.find("Type error") != std::string::npos) return "type checking";
    if (prefix.find("HIR lowerer") != std::string::npos) return "HIR lowering";
    if (prefix.find("LLVM codegen") != std::string::npos) return "LLVM code generation";
    if (prefix.find("comptime") != std::string::npos || prefix.find("comptime") != std::string::npos) return "compile-time evaluation";
    return "compilation";
}

std::string genericHint(const std::string& message) {
    const std::string lower = [&] {
        std::string s = message;
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }();

    if (lower.find("cannot open '") != std::string::npos)
        return "Check that the path exists and that you have permission to read the source file.";
    if (lower.find("expected ':'") != std::string::npos)
        return "Add ':' at the end of this declaration or control-flow header.";
    if (lower.find("expected '='") != std::string::npos)
        return "Add '=' followed by the value you want to assign.";
    if (lower.find("expected ','") != std::string::npos)
        return "Separate the preceding items with a comma.";
    if (lower.find("expected ')'" ) != std::string::npos)
        return "Check that the opening '(' has a matching ')' and that the call or expression is complete.";
    if (lower.find("expected ']'" ) != std::string::npos)
        return "Check that the opening '[' has a matching ']' and that the array, slice, or generic arguments are complete.";
    if (lower.find("expected 'in'") != std::string::npos)
        return "Use the form 'for name in collection:' for a loop.";
    if (lower.find("expected identifier") != std::string::npos || lower.find("expected name") != std::string::npos)
        return "Provide an identifier here, such as a variable, function, field, or type name.";
    if (lower.find("expected an indented block") != std::string::npos)
        return "Indent the statements belonging to this block consistently.";
    if (lower.find("tabs are not allowed") != std::string::npos)
        return "Replace the tab with spaces. Stable uses spaces for indentation.";
    if (lower.find("inconsistent indentation") != std::string::npos)
        return "Match the indentation level of the surrounding block. Use spaces consistently.";
    if (lower.find("unknown type '") != std::string::npos)
        return "Use a built-in type or a type declared in the current module.";
    if (lower.find("executable builds require a 'main' function") != std::string::npos)
        return "Add a function named 'main' with an i32 return type to make this module executable.";
    if (lower.find("unknown function '") != std::string::npos)
        return "Check the function name and make sure the function is declared before compiling this module.";
    if (lower.find("unknown local '") != std::string::npos)
        return "Declare the variable first, or check the spelling and scope of the name.";
    if (lower.find("unknown assignment target '") != std::string::npos)
        return "Declare this variable before assigning to it, or assign through a valid field/index.";
    if (lower.find("not assignable") != std::string::npos)
        return "Assign only to a variable, mutable field, or mutable indexable value.";
    if (lower.find("cannot assign to const") != std::string::npos || lower.find("modifies const") != std::string::npos)
        return "Create a mutable binding instead, or remove the assignment.";
    if (lower.find("while it is borrowed") != std::string::npos || lower.find("while it has a shared borrow") != std::string::npos)
        return "End the borrow before mutating the value, or use an exclusive EditView/&mut access when appropriate.";
    if (lower.find("read-only reference") != std::string::npos || lower.find("read-only view") != std::string::npos)
        return "Use &mut or EditView when mutation is intentional and no other borrow is active.";
    if (lower.find("moves a value") != std::string::npos || lower.find("use of moved") != std::string::npos)
        return "Borrow the value (&value) when ownership should be preserved, or keep using the owner before moving it.";
    if (lower.find("expected ") != std::string::npos && lower.find(", got ") != std::string::npos)
        return "Make the value's type match the expected type, or add an explicit cast such as 'value as u64'.";
    if (lower.find("same numeric type") != std::string::npos || lower.find("same type") != std::string::npos)
        return "Convert one operand explicitly so both operands have the same type.";
    if (lower.find("logical operators require bool") != std::string::npos)
        return "Use a boolean expression here, for example 'x != 0', rather than an integer value.";
    if (lower.find("condition must be bool") != std::string::npos)
        return "Make the condition boolean, for example by comparing the value with 0 or another value.";
    if (lower.find("division by zero") != std::string::npos)
        return "Ensure the divisor cannot be zero before this operation.";
    if (lower.find("array index must be an integer") != std::string::npos)
        return "Use an integer index such as usize, u32, or u64.";
    if (lower.find("fixed-array index is out of bounds") != std::string::npos)
        return "Keep the index in the range 0 .. length-1.";
    if (lower.find("slice lower bound exceeds upper bound") != std::string::npos)
        return "Swap the bounds so the slice starts no later than it ends.";
    if (lower.find("return") != std::string::npos && lower.find("fall through") != std::string::npos)
        return "Return a value on every possible control-flow path, or make the function return void.";
    if (lower.find("missing return value") != std::string::npos)
        return "Add the value that this non-void function should return.";
    if (lower.find("outside a loop") != std::string::npos)
        return "Move this statement inside a while/for loop, or remove it.";
    if (lower.find("non-void function may fall through") != std::string::npos)
        return "Add a return statement to every path that can reach the end of the function.";
    if (lower.find("cannot cast") != std::string::npos)
        return "Cast only between compatible numeric/boolean types.";
    if (lower.find("array literal") != std::string::npos && lower.find("elements") != std::string::npos)
        return "Provide exactly the number of elements required by the fixed-size array type.";
    if (lower.find("not an arena") != std::string::npos)
        return "Pass a value whose type is Arena to the 'in arena' allocation expression.";
    if (lower.find("unknown enum variant") != std::string::npos || lower.find("has no variant") != std::string::npos)
        return "Use one of the variants declared by that enum, including the exact spelling.";
    if (lower.find("non-exhaustive") != std::string::npos)
        return "Add a match arm for every possible enum/Option/Result case.";
    if (lower.find("out of memory") != std::string::npos)
        return "Reduce the requested allocation size or handle the allocation failure at this boundary.";
    return "Read the highlighted source line first; the compiler marks the construct that needs to change.";
}

} // namespace

ParsedError parseErrorMessage(const std::string& raw) {
    ParsedError out;
    const std::string text = trim(raw);

    std::smatch match;
    static const std::regex atColumn(R"((.*?)(?: at )([0-9]+):([0-9]+)(?::\s*|\s+)(.*))");
    static const std::regex atLine(R"((.*?)(?: at line )([0-9]+)(?::\s*|\s+)(.*))");
    static const std::regex parenLine(R"((.*?\(line )([0-9]+)\)(?::\s*|\s+)(.*))");

    if (std::regex_match(text, match, atColumn)) {
        out.phase = phaseFromPrefix(match[1].str());
        out.line = std::stoi(match[2].str());
        out.column = std::max(1, std::stoi(match[3].str()));
        out.message = trim(match[4].str());
    } else if (std::regex_match(text, match, atLine)) {
        out.phase = phaseFromPrefix(match[1].str());
        out.line = std::stoi(match[2].str());
        out.column = 1;
        out.message = trim(match[3].str());
    } else if (std::regex_match(text, match, parenLine)) {
        out.phase = phaseFromPrefix(match[1].str());
        out.line = std::stoi(match[2].str());
        out.column = 1;
        out.message = trim(match[3].str());
    } else {
        out.phase = phaseFromPrefix(text);
        out.message = text;
    }

    out.hint = genericHint(out.message);
    return out;
}

std::string render(const ParsedError& error, const std::string& path, const std::string& source) {
    std::vector<std::string> lines;
    std::stringstream stream(source);
    std::string line;
    while (std::getline(stream, line)) lines.push_back(line);
    if (source.empty() || (!source.empty() && source.back() == '\n')) {
        if (lines.empty() || source.back() == '\n') lines.push_back("");
    }

    const int totalLines = static_cast<int>(lines.size());
    const int lineNo = error.line > 0 && error.line <= totalLines ? error.line : 0;
    const int column = std::max(1, error.column);

    int width = 1;
    if (lineNo > 0) width = static_cast<int>(std::to_string(lineNo).size());

    std::ostringstream out;
    out << path;
    if (lineNo > 0) out << ':' << lineNo << ':' << column;
    out << ": error: " << error.message << '\n';

    if (lineNo > 0) {
        out << "  --> " << path << ':' << lineNo << ':' << column << '\n';
        out << std::string(static_cast<std::size_t>(width + 3), ' ') << "|\n";
        std::string displayedLine = lines[static_cast<std::size_t>(lineNo - 1)];
        if (!displayedLine.empty() && displayedLine.back() == '\r') displayedLine.pop_back();
        out << ' ' << std::setw(width) << lineNo << " | " << displayedLine << '\n';
        out << std::string(static_cast<std::size_t>(width + 3), ' ') << "| ";
        const std::size_t caretPos = static_cast<std::size_t>(std::max(0, column - 1));
        const std::string sourceLine = displayedLine;
        const std::size_t capped = std::min(caretPos, sourceLine.size());
        out << std::string(capped, ' ') << '^';
        if (error.message.size() < 96) out << "~~~~";
        out << '\n';
        out << std::string(static_cast<std::size_t>(width + 3), ' ') << "|\n";
        out << std::string(static_cast<std::size_t>(width + 3), ' ') << "= help: " << error.hint << '\n';
    } else {
        out << "  = " << error.phase << " failed before a source location was available\n";
        out << "  = help: " << error.hint << '\n';
    }
    return out.str();
}

} // namespace stable::diagnostics
