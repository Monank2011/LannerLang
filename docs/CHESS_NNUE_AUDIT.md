> **Historical document.** This audit records the state of Lanner 1.0.0. The current version is **Lanner 3.0.0**; see [CHANGELOG.md](../CHANGELOG.md) for what changed.

# Chess Engine and NNUE Hosting Audit

Lanner 1.0.0 was audited as a host language for native chess engines and NNUE inference before the Version 1 release gate.

## Result

**PASS for Version 1 application development.** The production LLVM compiler can compile and execute representative chess-engine and NNUE kernels written in Lanner without requiring the compiler implementation itself to be rewritten in Lanner.

## Chess-engine capability tested

`examples/chess_engine_kernel_audit.lan` exercises:

- `u64` bitboards and integer bitwise/shift arithmetic
- a 64-entry static attack table
- indexed fixed-array lookup
- `Cpu.ctz()` and `Cpu.popcount()`
- mutable scalar state
- recursive search/negamax-style control flow
- compile-time constants
- native `-O3` compilation

The existing `examples/uci_engine.lan` separately exercises interactive UCI stdin, `stop` polling, threads, atomics, and monotonic search deadlines.

## NNUE capability tested

`examples/nnue_host_audit.lan` exercises a representative quantized network with the same dimensions as the planned Lanner NNUE shape:

`45,192 inputs -> 16 accumulator lanes -> 32 hidden units -> 1 output`

The test loads a deterministic binary weight blob using `FS.read()` and then, entirely from Lanner code:

- obtains an unsafe byte pointer
- reinterprets it as `*i8` / `*i16` / `*i32`
- performs pointer arithmetic
- builds sparse first-layer accumulators
- applies clipped ReLU
- evaluates hidden/output layers
- returns a deterministic expected score

The supplied audit weight fixture is approximately 724 KB. The test's real purpose is the memory/data path and NNUE-shaped compute, not trained playing strength.

## Low-level engine facilities already available

The systems/runtime work provides:

- raw pointers and explicit `unsafe`
- manual/aligned allocation
- volatile and unaligned access
- static/global tables
- target-sized integers
- inline assembly
- CPU feature probing
- BMI2 PEXT/PDEP
- POPCNT/CTZ/CLZ
- prefetch
- SIMD vector values
- threads and atomics
- monotonic clocks/time budgets
- binary file buffers and filesystem I/O
- C ABI FFI
- native target selection and `-O3`

## Development-tree performance status

The development tree now provides typed signed integer vectors across 128-, 256-, and 512-bit widths, with aligned/unaligned loads and stores, i8-to-i16 widening, pairwise i16 multiply-adds, and i32 reductions. LLVM remains responsible for final ISA selection, so portable builds can use the generic vector path while deployment-specific `--cpu`/`--features` builds can expose wider machine vectors.

Therefore:

- **Correct/functional NNUE in pure Lanner:** PASS
- **High-performance vectorized NNUE entirely in Lanner:** PASS
- **ISA-specific hand-tuning beyond the exposed primitives:** still available through explicit unsafe assembly/FFI

The development tree is therefore suitable for implementing a complete native NNUE evaluator without C/C++ source in the evaluator itself.

## Bootstrap status

The compiler is not yet fully self-hosted. The production parser/typechecker/LLVM backend/driver remain C++, while the Lanner-written self-host compiler currently covers a supported subset and has a verified recursive fixed point.

This does **not** prevent Lanner applications from being written in Lanner. Full bootstrap can therefore remain the Version 2 compiler milestone.


## NNUE-oriented SIMD in Lanner

The development tree adds typed integer vectors for quantized inference: `i8x16`, `i16x8`, `i32x4`, `i8x32`, `i16x16`, and `i32x8`. Raw-pointer loads/stores are available in aligned and explicitly unaligned forms.

`Cpu.sxtLoI8x32` / `Cpu.sxtHiI8x32` widen signed 8-bit halves to signed 16-bit vectors. `Cpu.maddI16x16` computes pairwise signed 16-bit multiply-adds into eight 32-bit lanes. LLVM remains responsible for final instruction selection, so a scalar or explicit-vector Lanner kernel can still be optimized for the selected `--cpu`/`--features` target.


## NNUE optimization surface

Version 1 development work adds explicit signed integer vectors for NNUE kernels, including 128-bit (`i8x16`/`i16x8`/`i32x4`), 256-bit (`i8x32`/`i16x16`/`i32x8`), and 512-bit (`i8x64`/`i16x32`/`i32x16`) forms. Lanner can load signed i8 weight/activation blocks, widen them to i16, perform pairwise i16 multiply-adds, and reduce i32 results. This lets an NNUE implementation stay entirely in Lanner while still exposing operations LLVM can lower to target vector instructions.
