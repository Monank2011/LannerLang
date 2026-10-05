# STABLE self-hosting bootstrap

STABLE now has a reproducible staged bootstrap. The C++ compiler is stage 0. `selfhost/compiler.st` is a Stable-written bootstrap compiler containing the frontend bridge, semantic analysis, compact HIR lowering, a safe control-flow optimizer, and a C++17 code-generation backend.

## Legacy C++ bootstrap reference

The older C++17-emitting bootstrap remains available through `tools/verify_selfhost_bootstrap.sh` as a compatibility/reference path. It is not the authoritative bootstrap backend.

## Native LLVM Stage 0 -> Stage 1

Build the reference compiler:

```sh
cmake -S . -B build -DSTABLE_BUILD_TESTS=ON
cmake --build build -j2
```

Build the Stable-written compiler with the production native backend:

```sh
./build/stablec selfhost/compiler.st --backend=llvm -o build/stage1-native-llvm
```

The resulting executable reads `STABLE_SELFHOST_INPUT` and emits native LLVM for the supported bootstrap subset. `tools/verify_selfhost_native_llvm.sh` uses this path recursively.

## Native Stage 1 -> Stage 2 -> Stage 3

## Native recursive fixed point

`tools/verify_selfhost_native_llvm.sh` is the authoritative bootstrap verifier. It builds stage 1 with the production `--backend=llvm`, then has the native Stable-written compiler emit LLVM for `selfhost/compiler.st` again to produce stage 2, and repeats once more for stage 3. The verifier requires stage-2/stage-3 LLVM and representative probe artifacts to be byte-identical and executes probes from all native stages.

The legacy C++17 bootstrap remains available as an explicit compatibility/reference path through `tools/verify_selfhost_bootstrap.sh`; that script creates or uses a dedicated `build-legacy` configuration with `STABLE_ENABLE_LEGACY_HIR=ON`.

For the legacy C++ compatibility path, run:

```sh
./tools/verify_selfhost_bootstrap.sh
```

The harness performs:

```text
stage 0 stablec
    -> selfhost/compiler.st
    -> stage 1 compiler
    -> selfhost/compiler.st
    -> stage 2 compiler
    -> selfhost/compiler.st
    -> stage 3 compiler
```

It requires stage-2 and stage-3 compiler source artifacts to be byte-identical, then checks generated probe artifacts and runtime behavior. It also compares representative behavior against binaries produced directly by the stage-0 C++ compiler and checks that a deliberate local-borrow escape is rejected.

## Native LLVM self-hosting track

The Stable-written compiler uses its native LLVM backend by default:

```sh
STABLE_SELFHOST_INPUT=examples/selfhost_native_llvm_replacement.st \
build/stage1-native-llvm > build/selfhost-native-llvm.ll
clang -c build/selfhost-native-llvm.ll -o build/selfhost-native-llvm.o
clang build/selfhost-native-llvm.o -o build/selfhost-native-llvm
```

This replacement layer currently targets integer/bool scalar functions, direct calls, explicit integer/bool casts, assignment, `if`, and `while`. It derives blocks from source-order subtree boundaries and gives each generated SSA value and CFG label a per-function numeric identity. Native `getEnv()` normalizes an absent environment variable to a non-null empty string, and `tools/verify_selfhost_native_llvm.sh` includes a regression probe for that behavior. The existing replacement probe runs this path through stage 1, validates the generated LLVM with Clang, and executes the result.

The native path is now the authoritative self-hosting backend for the implemented surface; the remaining work is extending typed semantic/HIR/backend coverage to the rest of the language.

## Self-hosted phases

**Semantic analysis** validates structural syntax and a bootstrap type/ownership subset. Composite types include arrays, references, `Result`, optionals, and user-declared aggregate names. Returning a borrow tied to a local is rejected.

**HIR lowering** records one compact record per source line. Each record contains the HIR kind, source span, indentation level, liveness, and a semantic type-class tag. The representation is deliberately compact and easy for Stable itself to manipulate.

**Optimization** currently performs safe unreachable-tail elimination. Structural headers are preserved so indentation remains authoritative. This is a bootstrap optimization pass, not yet the full production optimizer.

**Backend** now has a production native LLVM path and an explicitly retained legacy C++17 bootstrap reference. The native self-host verifier builds stage 1 with `--backend=llvm`, then recursively rebuilds stage 2 and stage 3 through Stable-written native LLVM emission.

## Boundary

The supported compiler subset now has an authoritative recursive native LLVM bootstrap. Feature parity outside that subset, including the production typed AST/HIR architecture and broader language surface, remains an explicit future expansion rather than being silently claimed as complete.
