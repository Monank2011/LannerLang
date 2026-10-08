# Lanner development-tree upgrade for Guy

This document describes the Guy/engine-hosting upgrades that ship as part of Lanner 3.0.0.

## Engine-hosting additions

- Top-level `import "file.lan"` composition with relative-path resolution, duplicate-load suppression, cycle detection, and missing-module diagnostics.
- Place-aware borrow checking for disjoint structure fields such as `board.white` and `board.black`.
- LLVM `noalias` on `&mut T` function parameters to preserve exclusive-access information for optimization.
- `Thread.spawnCtx(function, context)` for explicit SMP worker contexts.

## Dynamic-array ergonomics

- Typed empty dynamic arrays can be written as `[]T` or `[]T.new()` without a dummy seed element.
- `[]T.with_capacity(n)`, `array.reserve(n)`, and `array.capacity()` provide explicit allocation control for engine/search workloads.

## NNUE additions

- Signed integer vectors: `i8x16`, `i16x8`, `i32x4`, `i8x32`, `i16x16`, `i32x8`, `i8x64`, `i16x32`, `i32x16`.
- Aligned and explicitly unaligned vector loads/stores.
- Signed i8-half widening to i16 vectors.
- Pairwise i16 multiply-add into i32 vectors.
- i32 horizontal reductions.
- 512-bit development-path coverage suitable for AVX-512-class targets.

## Reproducible coverage

The source tree includes regression examples for module imports, thread contexts, disjoint field borrows, `noalias`, 256-bit NNUE SIMD, and 512-bit NNUE SIMD code generation.

All 105 registered CTest cases were validated in smaller batches because of the execution environment's command-time limit.
