# Step 1 Basic-Compute Benchmark Bug Fixes

The Step 1 scalar benchmark surfaced three numeric-context/backend defects in the C++ production compiler used to bootstrap Stable:

1. **f32 LLVM literal emission**
   The LLVM backend emitted ordinary decimal text for `float` constants. LLVM 17's IR parser rejects those decimal constants for the `float` type. The backend now emits the exact LLVM hexadecimal floating representation. For f32 values, the source decimal is rounded to IEEE-754 f32 first and the resulting value is represented using the LLVM-compatible widened bit pattern.

2. **Negative f32 literal context**
   Unary `-`/`+` previously inferred their operand without the surrounding expected numeric type. This caused a declaration such as `a: f32 = -0.9` to annotate the literal as f64 and later arithmetic could report `expected f64, got f32`. Unary numeric checking now propagates the expected numeric type into its operand.

3. **Negative i64 literal context**
   The same unary-context issue caused negative integer literals such as `s: i64 = -918273645` to retain the default i32 literal type. Numeric unary checking now propagates the expected integer type, allowing the literal magnitude to be represented and used as i64.

## Regression coverage

Added:

- `examples/f32_numeric_literals.st`
- `examples/i64_negative_literal.st`
- Production integration assertions for both typed negative literal cases.
- CTest runtime checks for both examples.

## Validation

- Default production CTest: **31/31 PASS**.
- Native self-host regression suite: **60/60 PASS**.
- Typed self-host frontend verifier: **PASS**.
- Legacy recursive bootstrap with HIR compatibility enabled: **PASS**.
- Stage-2/stage-3 bootstrap hashes: byte-identical.
- The repaired f32 example produces the same exit result as an equivalent C++17/Clang 17 translation: **42**.
- Repaired f32 LLVM IR assembles successfully with Clang/LLVM 17.

## LLVM toolchain portability hardening

The floating-point literal emitter is treated as an LLVM IR portability concern rather than an LLVM 17-specific workaround. All `f32` and `f64` literals use LLVM’s exact 16-digit hexadecimal IEEE-754 spelling, avoiding the assembler exact-decimal restriction that rejects values such as `0.9`. The driver discovers the selected `clang`/LLVM major version through `STABLE_CLANG` or `LLVM_CC` and enforces the modern opaque-pointer compatibility floor of LLVM 15.
