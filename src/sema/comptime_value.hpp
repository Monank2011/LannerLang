#pragma once
#include <variant>
#include <string>
#include <cstdint>

// A value computed at compile time — int/float/bool/string only for now.
// (No comptime arrays/structs yet; that's a deliberate v1 scope cut.)
using ComptimeValue = std::variant<int64_t, double, bool, std::string>;