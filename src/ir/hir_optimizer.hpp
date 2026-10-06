#pragma once

#include "hir.hpp"

namespace lanner::hir {

enum class OptimizationLevel { O0, O1, O2 };

// Optimizes typed HIR without changing safety semantics. O1 performs local
// constant folding. O2 additionally removes provably redundant bounds checks,
// forwards local stores into loads within a basic block, removes unreachable
// blocks, and eliminates unused pure instructions.
void optimize(Module& module, OptimizationLevel level = OptimizationLevel::O2);

} // namespace lanner::hir
