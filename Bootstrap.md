# **Lanner 1.0.0 Bootstrap**

Lanner contains a Lanner-written compiler track and recursive self-hosting verification.

## **Production path**

```text
Lanner source
    |
    v
C++ bootstrap compiler
    |
    v
LLVM IR
    |
    v
Clang/LLVM
    |
    v
Native executable
```

## **Native self-hosting**

The Lanner-written compiler is located under **`selfhost/`**.

The native LLVM self-hosting verifier compiles the Lanner-written compiler through the Lanner compiler and compares the resulting stages.

The validated production path reaches a **byte-identical stage-2/stage-3 fixed point**.

## **Legacy HIR**

The legacy HIR/SSA path is optional:

```sh
cmake -S . -B build \
    -DLANNER_ENABLE_LEGACY_HIR=ON \
    -DLANNER_BUILD_TESTS=ON
```

It remains useful for bootstrap/reference testing and IR inspection.

The production `auto` backend uses the **native LLVM path**.

## **Verification tools**

Important scripts include:

```text
tools/verify_selfhost.sh
tools/verify_selfhost_bootstrap.sh
tools/verify_selfhost_native_llvm.sh
tools/verify_selfhost_typed_frontend.sh
tools/verify_native_selfhost.sh
tools/verify_llvm_compat.sh
tools/verify_native_readfile_failure.sh
```

LLVM toolchain selection is available through **`LANNER_CLANG`** and **`LLVM_CC`**.


## **Current bootstrap boundary**

The recursive native bootstrap is a **supported-subset fixed point**, not a claim that the entire 1.0 compiler is self-hosted. The full production compiler remains implemented in C++ for the authoritative lexer, parser, typechecker, comptime evaluator, native LLVM code generator, command-line/toolchain driver, and mobile/game project generators. The optional legacy HIR implementation is also C++.

The Lanner-written compiler currently does not cover the Step 2-7 runtime namespaces (Web, backend/cloud, ML/AI, Mobile, Game, and DevOps) or the complete systems/unsafe/toolchain surface.
