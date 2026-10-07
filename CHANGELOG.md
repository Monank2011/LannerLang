# Changelog

## 2.0.0 — Chess/NNUE development release

- Published the fixed `.lan` source tree as LannerLang 2.0.0.
- Added module imports, disjoint-field borrow support, exclusive `&mut`/LLVM `noalias` lowering, and explicit thread contexts.
- Added NNUE-oriented integer SIMD types, widening, multiply-add, reductions, and AVX-512 development coverage.
- Corrected POSIX runtime filesystem access to use `st.st_mode` and `st.st_size`.
- Added chess-engine and NNUE examples plus deterministic regression fixtures.


Lanner was created and is developed by **Monank Gohil**, who began developing the language at age **15**.

## **1.0.0 — Initial packaged release**

### Toolchain installer update

- Fixed the pip bootstrapper to use the selected release asset's actual filename.
- Added optional `GITHUB_TOKEN` support for GitHub API rate-limit resilience.
- Added Linux and Windows installers that download official LLVM/Clang **23.1.2**.
- Pinned and verified the Linux and Windows archive SHA-256 checksums.
- Installs LLVM side by side under Lanner's own versioned directory.
- Configures the Lanner compiler wrapper through `LANNER_CLANG` without replacing system LLVM.


- Published the complete Lanner compiler and language source tree.
- Added bold, release-focused README presentation and direct binary download links.
- Added Linux x86_64 and Windows x86_64 compiler packages.
- Added package installation notes under `docs/release/`.
- Linux compiler build completed successfully; 98/99 automated tests passed in the release environment.
- The remaining i386 test assertion expects `Intel i386`, while the host `file` utility reports the equivalent `Intel 80386` architecture.

# Changelog

## 1.0.0 chess/NNUE readiness audit

- Added permanent chess-engine hosting coverage for `u64` bitboards, static attack tables, POPCNT/CTZ, recursive search control, and native `-O3` compilation.
- Added a representative 45,192 → 16 → 32 → 1 quantized NNUE inference fixture using binary weight loading, unsafe typed pointer access, sparse feature accumulation, clipped ReLU, and output evaluation.
- Added `docs/CHESS_NNUE_AUDIT.md` documenting the Version 1 engine/NNUE gate and the remaining typed-SIMD ergonomics caveat.

## 1.0.0 backend/cloud completion

- Added hosted TCP/UDP sockets with blocking/nonblocking controls, TCP_NODELAY, shutdown, error reporting, and local/peer port queries.
- Upgraded the Linux readiness poller to epoll with a portable WSAPoll/poll fallback.
- Added Mutex, RwLock, Condvar, and Semaphore synchronization primitives.
- Reworked Process.spawn to create real OS child-process handles; added pid and terminate operations while keeping output capture separate in Process.output.
- Added HTTP/1.1 GET and JSON POST client operations with bounded response parsing, Content-Length handling, chunked decoding, and connection-close fallback.
- Added JSON syntax validation/serialization and owned growable Buffer primitives.
- Added backend/cloud examples and cross-platform CTest coverage.

## 1.0.0 systems/low-level completion

- Added explicit unsafe raw pointers (`*T`, `*mut T`), raw-address creation, dereference, unchecked indexing, pointer arithmetic, pointer/integer casts, `ptrDiff()`, and target-sized `usize`/`isize`.
- Added manual/aligned/stack allocation plus volatile, unaligned, and byte-memory primitives.
- Added packed layouts, explicit static alignment/sections/thread-local storage, and external C global declarations.
- Added unsafe function-pointer types so unsafe call requirements survive function values and callbacks.
- Added `asmI32`/`asmPtr` alongside existing inline assembly, with explicit unsafe enforcement.
- Added target-aware `--target`, `--sysroot`, `--cpu`, `--features`, linker/linker-script controls, object/assembly output, explicit freestanding entry points, and no-runtime executable builds.
- Added freestanding x86-64, i386, and AArch64 cross-object coverage, C-global FFI, qsort callbacks, C aggregate pointer FFI, and low-level regression coverage.
- Made by-value aggregate parameters/returns in `extern` functions a checked error unless represented through raw pointers or an explicit C wrapper, preventing target-specific ABI mismatches from becoming silent miscompiles.
- The complete regression suite now passes 55/55 tests, including a genuinely runtime-free freestanding executable.
- Systems regression suite: **all tests passing**.

## 1.0.0 engine stack completion

- Added interactive `Stdin.readLine()` / `Stdin.hasInput()` support with string protocol helpers: `startsWith`, `equals`, `parseU64At`, and checked string indexing.
- Added real compile-time-initialized `static` / `static mut` global storage for lookup tables and engine state.
- Added explicit `extern` C ABI declarations and repeatable `--link` arguments for external objects/libraries.
- Added engine CPU primitives for feature probing, `rdtsc`, popcount/ctz/clz/bswap, BMI2 PEXT/PDEP, prefetch, and fixed-width v128/v256/v512 vector loads/stores/operations.
- Added native OS threads with `Thread.spawn/join/detach`, hardware-concurrency queries, yielding, and `Atomic[T]` load/store/fetch/CAS operations.
- Added a monotonic engine clock with nanosecond deadlines, expiration, remaining-budget, and sleep APIs.
- Added the cross-platform runtime source, engine-stack examples, and regression coverage.
- Production CTest: **43/43 passing**.

## 0.6.0-dev bootstrap step 21

- Hardened LLVM toolchain portability after the scalar benchmark: the native driver now discovers the selected Clang/LLVM major version, supports `LANNER_CLANG` and `LLVM_CC`, rejects pre-LLVM-15 toolchains with an explicit opaque-pointer diagnostic, and adds a reusable LLVM compatibility smoke-test. Floating-point literals remain emitted in LLVM's exact 16-digit hexadecimal IEEE-754 form so non-exact decimal source literals are preserved across the supported modern LLVM range.
- Completed the current implemented-language parity pass across production and native self-hosted LLVM paths.
- Promoted local `comptime` declarations to production semantic/codegen support and added comptime-sized fixed-array coverage.
- Extended typed-HIR constant folding to `f32`/`f64`, including arithmetic, ordered comparisons, unary operations, and safe scalar casts.
- Added native self-host regression coverage for comptime-sized arrays and forward/mutual recursion.
- Confirmed the C++ bootstrap emitter's two-pass nominal/prototype/body layout removes the former function textual-order restriction.
- Native self-host regression coverage is now **60/60 passing**.
- Production CTest is now **50/50 passing**; recursive stage-1 -> stage-2 -> stage-3 bootstrap remains byte-identical.

## 0.6.0-dev bootstrap step 20

- Added the official Lanner logo as a transparent high-resolution editor asset.
- Expanded the self-hosted native LLVM path with typed `f32`/`f64` function signatures, arithmetic, comparisons, locals, arrays, `.push()`, `for` iteration, and float printing.
- Added native `for` lowering for dynamic/fixed arrays and `View`/`EditView`, including typed scalar loop bindings and correct `break`/`continue` targets.
- Added native compile-time value handling for `comptime` declarations, including integer expression evaluation and runtime use of comptime bool/float/string values; local comptime-sized fixed arrays are supported as a native extension.
- Fixed native `print(bool)` LLVM emission to use an SSA `select` instruction instead of an obsolete constant-expression `select` embedded inside a call argument.
- Added regression coverage for `else if`, binary/hex integer literals, unary/bitwise operators, floats, `for`, `print`, and comptime behavior.
- Native self-host regression coverage is now **58/58 passing**.
- Added `tools/lanner-vscode/`, a VS Code extension contribution for the `lanner` language and `.lan` files.
- Added a `Lanner File Icons` theme that maps `.lan` files and the `lanner` language ID directly to the Lanner logo.
- Added the Lanner logo to the language contribution so compatible file icon themes can use it as the language default icon.
- Added editor-branding documentation and the canonical logo asset under `docs/assets/`.

## 0.6.0-dev bootstrap step 19

- Promoted the typed AST/module -> typed-HIR -> native LLVM pipeline to the unconditional production path of `selfhost/compiler.lan`.
- Native self-hosting no longer requires `LANNER_SELFHOST_NATIVE_LLVM`; a fresh Lanner-written compiler invocation emits native LLVM by default.
- Expanded typed declaration registries so native LLVM consumes typed function parameters and struct fields directly instead of reparsing declaration lists from source text.
- Completed the production boundary for the self-hosted compiler: the old semantic/C++/HIR bridge remains only behind explicit `LANNER_SELFHOST_REFERENCE=1` for differential/reference validation.
- Added the typed-front-end verifier and kept aggregate type/provenance rejection coverage in the authoritative native bootstrap.
- Native fixed-point validation: stage-2/stage-3 LLVM is byte-identical.
- Production CTest: **29/29**.
- Native typed frontend verification: **PASS**.

## 0.6.0-dev bootstrap step 18

- Extended the typed-HIR optimizer with provably safe fixed-array bounds-check elimination for compile-time-known indices.
- Added same-basic-block local load forwarding from a preceding `StoreLocal`, with whole-function use analysis so values used across CFG edges are never invalidated by the optimization.
- Added conservative memory barriers around calls, indirect stores, dynamic-array mutation, and destruction so load forwarding never assumes away possible aliasing side effects.
- Added optimizer regressions covering redundant loads, fixed-array bounds checks, and call invalidation.
- Promoted the native LLVM bootstrap to the authoritative recursive path: stage 0 builds stage 1 with `--backend=llvm`, stage 1 builds stage 2, and stage 2 builds stage 3.
- Added native stage-1/stage-2/stage-3 artifact and behavior comparisons to the native bootstrap harness.
- Kept the legacy C++/HIR bootstrap as an explicitly isolated compatibility/reference path with its own `build-legacy` configuration.
- Validation: production CTest **29/29**, legacy CTest **49/49**, optimizer regression **PASS**, native self-host **45/45**, native recursive fixed point **PASS** with identical stage-2/stage-3 LLVM, and missing-file readFile failure **PASS**.

## 0.6.0-dev bootstrap step 17

- Added recursive aggregate storage-provenance metadata to the semantic type system and typed HIR, covering nested `View`/`EditView` and arena-backed ownership dependencies.
- Added escape checks for aggregates returned, assigned, or inserted into owning collections so embedded short-lived borrows/regions cannot silently escape.
- Preserved scalar field/index extraction from aggregates without propagating irrelevant borrow provenance.
- Distinguished local arenas from caller-owned arena parameters, allowing safe region-carrying returns when the region itself comes from the caller.
- Added HIR and memory-model regressions for nested View escape, nested arena escape, scalar projection, and parameter-owned Arena returns.
- Verified the bootstrap fixed point and native self-host path remain lanner after the provenance migration.

## 0.6.0-dev bootstrap step 16

- Reworked native LLVM ownership cleanup around typed recursive drop glue for structs, dynamic/fixed arrays, Optional, Result, and arenas.
- Added deep ownership classification for nested aggregates and propagated ownership moves through calls, returns, constructors, assignments, Result/Optional guards, and owning-element `push()`.
- Hoisted owning locals now begin in an inert zero-initialized state, so function-level cleanup is safe even when initialization occurs only on a branch.
- Hardened cleanup dispatch to fail closed for non-owning or unknown type codes instead of emitting an unrelated destructor.
- Fixed native arena creation/allocation and arena-aware dynamic-array growth while preserving lanner addresses.
- Added native regressions for nested destruction, nested assignment replacement, owning `push()`, arena allocation, and Result ownership.
- Native selfhost suite: **45/45 passing**. AddressSanitizer ownership stress cases are clean.
- Verified recursive native stage-1 -> stage-2 -> stage-3 compilation with **byte-identical stage-2/stage-3 LLVM and object artifacts**.
- Increased the bootstrap CTest timeout to 180 seconds to accommodate the larger self-hosted compiler while preserving the same fixed-point comparison.

## 0.6.0-dev bootstrap step 15

- Added native LLVM enum type resolution and enum-variant value lowering, including enums used as `Result` payloads and typed locals.
- Removed the stale native function-signature rejection of top-level enum declarations.
- Extended native scalar storage/load/store and local-hoisting paths to carry enum values as zero-cost `i64` representations.
- Hardened native `readFile()` failure handling with checked open/seek/tell/rewind/allocation/read paths, cleanup, overflow protection, and an explicit LLVM trap on failure.
- Added native selfhost regression coverage for enum `Result` payloads, successful `readFile()`, and missing-file failure behavior.
- Native selfhost regression coverage is now **38/38 passing**; final CTest validation is **49/49 passing across the final test groups**, including byte-identical stage-2/stage-3 bootstrap output.


## 0.6.0-dev bootstrap step 14

- Added native LLVM lowering for `View[T]` and `EditView[T]` as pointer-plus-length aggregates.
- Added slicing from fixed and dynamic arrays, including full/open-ended slices and view reslicing.
- Added native View/EditView indexing, deterministic bounds traps, `EditView` mutation, `.len()`, and `.isEmpty()`.
- Added View/EditView function parameter and return support with regression coverage for fixed/dynamic backing arrays and mutation.
- Fixed slice parsing so struct literals are not misclassified as slices.
- Kept aggregate registry metadata recursion-safe and removed temporary debug instrumentation from the native block emitter.
- Expanded native selfhost regression coverage to 36/36 programs; full CTest remains 47/47 passing.
- Verified recursive native stage-2 -> stage-3 LLVM remains byte-identical.


## 0.6.0-dev bootstrap step 13

- Added first-class native LLVM Optional aggregates with `none`, contextual `T -> T?` lifting, parameter/return support, and fixed/dynamic aggregate payloads.
- Added first-class native LLVM Result aggregates with `Ok(...)`/`Err(...)`, parameter/return support, and `is Ok(...)` / `is Err(...)` guard bindings.
- Fixed recursive native aggregate lowering by reusing matching Optional/Result values before payload lifting and by copying registry metadata before recursive emission.
- Fixed Result guard control flow so a terminating matched branch does not incorrectly terminate the non-matching continuation block.
- Corrected native `readFile` array metadata to match `[]u8`.
- Expanded native selfhost regression coverage to 28/28 programs; full CTest remains 47/47 passing.
- Verified recursive native stage-2 -> stage-3 compilation produces byte-identical LLVM artifacts.

## 0.6.0-dev bootstrap step 5

- Hardened the self-hosted semantic keyword matcher so `returnX`, `breakfast`, and `continueX` are not misclassified as control-flow keywords; this allows `compiler.lan` to self-parse through its own semantic gate.
- Lowered enum members inside real C++ `enum class` definitions instead of emitting them as unrelated local assignments; the bootstrap Result/enum stress program now compiles and preserves enum references such as `ErrorCode.Empty`.
- Made the CTest self-hosting checks inherit `LANNER_BUILD_DIR` from the active build tree, so the bootstrap scripts work from non-default build directories too.
- Corrected the C++ bootstrap struct-field `.len()` adaptation to use `size_t`, matching `usize` fields such as `LLVMFunctionSig.paramStart`.
- Verified the complete legacy compatibility suite at 47/47, the stage-1 -> stage-2 -> stage-3 bootstrap with byte-identical compiler artifacts, and the native LLVM replacement probe.

## 0.6.0-dev memory-safety + output milestone

- Fixed the move/borrow hole where an owning move could proceed while a reference to the source remained live; borrowed owners are now rejected at every implicit ownership-transfer site.
- Preserved exclusive `EditView` provenance across function-return boundaries, so a returned view keeps the original owner exclusively borrowed.
- Enforced `EditView` exclusivity for ordinary owner reads as well as mutations, while preserving normal `&mut` reference behavior.
- Fixed HIR constant-folded signed widening so printing negative narrow integers matches the direct LLVM backend.
- Added the built-in `print(...)` statement for strings, booleans, all built-in integer types, and `f32`/`f64`, with newline output on both LLVM backends and in the self-hosted C++ bootstrap path.
- Updated `examples/hello.lan` to the canonical `print("Hello, LANNER!")` first-program example and added memory/optimizer regression coverage.
- Full legacy-HIR compatibility suite passes 47/47 tests, including stage-2/stage-3 bootstrap and native LLVM replacement checks.

## 0.5.0-dev

- Added the first real Lanner-in-Lanner compiler pipeline: Lanner-written semantic validation, compact typed HIR lowering, safe unreachable-tail optimization, and a C++17 bootstrap backend.
- Added reproducible stage-1 -> stage-2 -> stage-3 bootstrap comparison, including byte-identical compiler artifacts and generated probe artifacts.
- Added stage-0 behavioral comparison and a deliberate borrow-escape semantic rejection regression.
- Hardened the self-hosted bootstrap HIR record layout to carry six fields: opcode, source span, indentation, liveness, and semantic type-class metadata.
- Started the native backend replacement track with a Lanner-written LLVM emitter for scalar integer/bool functions, direct calls, explicit casts, assignment, `if`, and `while`.
- Reworked native expression results to scalar IDs with per-function SSA numbering, eliminating large aggregate-return dependence in the bootstrap path.
- Added deterministic native LLVM replacement regression coverage that stage-1 emits, Clang validates, and executes successfully.

- Expanded typed HIR/SSA to native structs and fixed arrays with aggregate field/index operations and bounds traps.
- Added HIR lowering for `Option`, `Result`, `Ok`/`Err`, `is Ok/Err`, and Result guards for supported value payloads.
- Added contextual `T -> T?` optional lifting across assignments and calls, including fixed and dynamic aggregate payloads.
- Tightened ownership transfer on returns and made non-copy Result guard bindings borrow rather than duplicate ownership.
- Added nested optional aggregate regression coverage for fixed and dynamic arrays.
- Expanded compiler validation during the intermediate aggregate phase to 32/32 tests, including strict warnings and ASan/UBSan leak checking.
- Completed the ownership-heavy HIR memory layer for dynamic arrays, arena-backed allocation, views/references, nested owning aggregates, and deterministic reverse-order destruction.
- Mirrored recursive ownership cleanup in the direct LLVM backend and added transfer rules for aggregate literals, `push`, field/index assignment, and moved temporaries.
- Rejected implicit moves out of non-copy field/index projections to prevent aliasing-driven double destruction.
- Added dedicated memory regression programs for both backends and raised the full suite to 41/41 tests.
- Whole-project `-Werror` and ASan/UBSan builds now pass the complete suite.

## 0.4-dev

- Added the native compiler driver: source -> type checking -> LLVM IR -> Clang -> executable.
- Added `lanner file.lan` executable builds and `lanner file.lan --run`.
- Added `-O0` through `-O3` driver options.
- Added `-o` output selection and LLVM IR file output.
- Added `--version` and updated project version metadata.
- Added CLI integration tests for checking, LLVM emission, executable builds, and launching.
- Improved driver handling for output directories and shell-safe filesystem paths.
- Kept readable source diagnostics with repair hints as the compiler boundary.



## 0.4.1-dev

- Added production backend selection with `--backend=auto|hir|llvm`.
- `auto` now prefers typed HIR/SSA for supported scalar/control-flow programs and falls back only for deliberate HIR capability gaps.
- Added HIR short-circuit lowering for `&&` and `||` through CFG blocks.
- Preserved exact source line/column metadata when cloning semantic types for backend diagnostics.
- Added aggregate auto-backend regression coverage.

### Self-hosting bootstrap

- Added a real Lanner-written arena-backed AST frontend at `selfhost/compiler.lan`.
- Added configurable bootstrap input through `getEnv("LANNER_SELFHOST_INPUT")` with a deterministic default.
- Added a representative self-hosting stress program covering enums, nested owning arrays, `Result`, `Ok`, `Err`, and guards.
- Added the stage-0 -> stage-1 bootstrap harness at `tools/bootstrap_selfhost.sh`.
- Added CTest coverage for default frontend execution, stress parsing, and parsing the compiler's own source.
- Fixed direct LLVM `getEnv` emission and corrected nullable empty-string IR spelling.
- Fixed Lanner parser slice recognition and `else if` parsing in the self-hosted frontend.

## 0.5.0-dev replacement phase: production backend split

- Native/direct LLVM is now the production `auto` backend; it no longer silently falls back to HIR.
- Legacy HIR is compile-time gated behind `LANNER_ENABLE_LEGACY_HIR=ON` and is excluded from the default production `lanner` build.
- HIR sources are linked only for the explicitly enabled legacy/bootstrap configuration.
- Direct LLVM builtin dispatch now includes `writeRaw`, `writeIntRaw`, and `writeByteRaw`, restoring parity needed by the Lanner-written compiler.
- Self-host bootstrap verification remains on the legacy HIR configuration until the Lanner-written native LLVM emitter reaches aggregate/runtime feature parity. This is intentional: the old path is now bootstrap infrastructure, not the production backend.

## Step 12: native fixed-array parity boundary

The native LLVM self-host backend now supports fixed arrays (`[N]T`) as first-class
aggregate values, including literal construction, local bindings, function
parameters and returns, indexed reads/writes, mutable fixed-array references,
and fixed-array fields inside structs. Fixed-array references use native LLVM
pointer lowering alongside the existing dynamic-array/struct reference model.

Validation: native selfhost probes 21/21, full CTest 47/47, and native LLVM
replacement verification pass.

## Unreleased numeric hardening

- Fixed LLVM 17 f32 constant emission in the direct LLVM backend.
- Fixed typed f32 negative literals and typed negative i64 literals by propagating numeric context through unary operators.
- Added regression tests and a focused basic-compute bugfix report.

### Step 2 memory benchmark repairs
- Fixed assignment through scalar `&mut T` bindings so `v = value` targets the pointee rather than rebinding the reference slot.
- Added `string.len()` lowering to the direct LLVM and legacy HIR backends.
- Added LLVM/HIR regression tests for both cases.

## Step 4: ML / AI

- Added native f32/f64 multidimensional tensor handles (rank 1..8).
- Added tensor shape/stride queries, element access, reshape/transpose/slice, elementwise arithmetic, scaling, activations, softmax, reductions, dot, matrix multiplication, and CPU NCHW convolution.
- Added explicit reverse-mode autodiff tapes for core tensor operations.
- Added CUDA/ROCm/Metal/BLAS availability detection.
- Added real CBLAS FFI regression coverage using `--link -lblas`.
- Added ML examples and documentation.

## Lanner 1.0.0 Mobile

Added native Android/iOS compilation targets, project generators, platform bridges, Mobile APIs, native UI shells, and mobile regression coverage.


## Lanner 1.0.0 Game Development

- Added native `Game` and `Graphics` APIs for SDL2 windows, events, input, 2D rendering, textures, audio and frame timing.
- Added dynamic native graphics-procedure loading for Vulkan, OpenGL and platform graphics APIs, with explicit unsafe boundaries for low-level calls.
- Added cross-platform `--game-project` generation with CMake, embedded Lanner runtime source and asset layout.
- Added a complete native game-loop regression using SDL dummy video/audio drivers.
