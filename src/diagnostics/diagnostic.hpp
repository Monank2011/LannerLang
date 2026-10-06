#pragma once

#include <string>

namespace lanner::diagnostics {

struct ParsedError {
    std::string phase = "compiler";
    std::string message;
    int line = 0;
    int column = 1;
    std::string hint;
};

ParsedError parseErrorMessage(const std::string& raw);
std::string render(const ParsedError& error, const std::string& path, const std::string& source);

} // namespace lanner::diagnostics
