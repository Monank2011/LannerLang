# Chess Engine and NNUE Hosting Audit

Lanner 1.0.0 was audited as a host language for native chess engines and NNUE inference before the Version 1 release gate.

## Result

**PASS for Version 1 application development.** The production LLVM compiler can compile and execute representative chess-engine and NNUE kernels written in Lanner without requiring the compiler implementation itself to be rewritten in Lanner.

## Chess-engine capability tested

`examples/chess_engine_kernel_audit.st` exercises:

- `u64` bitboards and integer bitwise/shift arithmetic
- a 64-entry static attack table
- indexed fixed-array lookup
- `Cpu.ctz()` and `Cpu.popcount()`
- mutable scalar state
- recursive search/negamax-style control flow
- compile-time constants
- native `-O3` compilation

The existing `examples/uci_engine.st` separately exercises interactive UCI stdin, `stop` polling, threads, atomics, and monotonic search deadlines.

## NNUE capability tested

`examples/nnue_host_audit.st` exercises a representative quantized network with the same dimensions as the planned Lanner NNUE shape:

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

## Known Version 1 performance caveat

Lanner's current built-in SIMD vector surface is intentionally small and exposes fixed-width `i64` lanes with arithmetic/bitwise operations. It does **not** yet provide the richer typed `i8`/`i16`/`i32` vector and widening/dot-product primitives normally used to hand-write an optimized NNUE inner loop.

Therefore:

- **Correct/functional NNUE in pure Lanner:** PASS
- **High-performance hand-vectorized NNUE entirely in Lanner:** PARTIAL
- **High-performance NNUE via Lanner + native C/ISA FFI:** PASS

The compiler can optimize scalar loops and can call native SIMD kernels through the existing explicit FFI. A richer typed SIMD/NNUE intrinsic layer is a sensible Version 1.x/Version 2 improvement, not a blocker to developing the engine and NNUE in Lanner.

## Bootstrap status

The compiler is not yet fully self-hosted. The production parser/typechecker/LLVM backend/driver remain C++, while the Lanner-written self-host compiler currently covers a supported subset and has a verified recursive fixed point.

This does **not** prevent Lanner applications from being written in Lanner. Full bootstrap can therefore remain the Version 2 compiler milestone.
