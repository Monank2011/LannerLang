# Lanner 1.0.0 Version 1 Release Audit

## Technical gate

**PASS for native application development.** The supplied Lanner 1.0.0 source tree can host representative workloads across the seven audited domains, and it can compile a representative chess engine kernel and NNUE inference path entirely from Lanner source.

### Chess/NNUE evidence

- UCI runtime example: compiles, runs, answers `uci`/`isready`, runs a timed worker, observes `stop`, and exits on `quit`.
- Chess engine kernel audit: 64-entry static attack table, `u64` bitboards, bitwise/shift operations, POPCNT/CTZ, recursive search, and native `-O3` execution all pass.
- NNUE audit: `45,192 -> 16 -> 32 -> 1` quantized inference path loads a deterministic binary weight blob and performs sparse accumulator, clipped activation, hidden-layer, and output computations entirely in Lanner source.
- Native `--cpu`/`--features` compilation works. LLVM auto-vectorizes the NNUE scalar kernel; a native assembly inspection on the audit host showed vectorized integer operations including `vpmaddwd`, with the exact instructions depending on selected target features.

## Compiler bootstrap gate

**PASS as a recursive supported-subset bootstrap, not as full self-hosting.** The Lanner-written compiler reaches a byte-identical stage-2/stage-3 LLVM fixed point and its current native selfhost corpus passes. The production parser, typechecker, LLVM code generator, and driver still remain C++ in Version 1. Full bootstrap is retained as the planned Version 2 compiler milestone.

## NNUE performance boundary

Lanner can implement a functional NNUE without C/C++ source in the NNUE itself. The current typed SIMD API is smaller than the hand-vectorized APIs commonly exposed by C++/Rust, so custom ISA-specific NNUE intrinsics are less ergonomic than they should be. LLVM's optimizer can still auto-vectorize suitable scalar loops, and explicit C ABI FFI remains available for specialized kernels.

## Distribution check

The release archive should include the project's authoritative license file before public publication. The supplied source archive used for this audit does not contain a top-level `LICENSE` file even though `README.md` references one. Do not invent or change licensing text during packaging; restore the project's intended license file from the canonical repository/release source.
