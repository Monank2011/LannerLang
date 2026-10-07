# Lanner 2.0.0 Bootstrap Audit

This audit records what is genuinely self-hosted in the Step 7 source tree and what still depends on the C++ production compiler.

## Executive result

Lanner has a reproducible **native Lanner-written compiler fixed point for a supported subset**:

```text
C++ stage 0
    -> Lanner-written stage 1
    -> Lanner-written stage 2
    -> Lanner-written stage 3
```

`tools/verify_selfhost_native_llvm.sh` currently reaches a byte-identical stage-2/stage-3 LLVM fixed point. The native selfhost regression currently passes **60/60** representative programs. The typed selfhost frontend verification also passes.

This is not yet full compiler self-hosting. Representative programs from the completed Systems, Web, Backend/Cloud, ML/AI, Mobile, Game, and DevOps layers are not accepted by the current Lanner-written compiler. This was checked against the stage-1 native selfhost compiler rather than inferred from documentation.

## Production C++ that remains

Approximate source line counts in this release tree:

| Subsystem | Language | LOC | Status |
| --- | --- | ---: | --- |
| Lexer | C++17 | 317 | Production compiler |
| Parser / AST construction | C++17 | 800 | Production compiler |
| Typechecker / ownership semantics | C++17 | 3,070 | Production compiler |
| Comptime evaluator | C++17 | 184 | Production compiler |
| Native LLVM code generator | C++17 | 4,032 | Production compiler |
| Compiler driver + target/link/project generation | C++17 | 1,106 | Production compiler |
| Diagnostics | C++17 | 197 | Production compiler |
| CLI entry point | C++17 | 279 | Production compiler |
| Legacy HIR implementation | C++17 | 3,221 | Optional/reference only |
| **Total C++ source** | | **13,206** | |
| **Non-legacy production C++** | | **9,985** | |
| Lanner-written selfhost compiler/lexer/parser | Lanner | 9,043 | Recursive supported-subset bootstrap |
| Hosted runtime | C11 | 2,721 | Native runtime, not C++ |

The largest remaining C++ components are therefore the native LLVM backend and typechecker, followed by parser and driver/toolchain code.

## What the Lanner-written compiler actually covers

The selfhost compiler contains a lexer, parser, semantic/type analysis, compact HIR, a native LLVM emitter, ownership/drop handling for its supported aggregate subset, and bootstrap probes. It can recursively compile itself and the current native-selfhost test corpus.

It does **not** currently implement the full production language surface or the full runtime namespace surface. In particular, the current selfhost implementation does not implement the production Web, Backend/Cloud, ML/AI, Mobile, Game, or DevOps built-in namespaces. It also does not provide the complete unsafe/assembly/ABI/target-driver surface introduced in the production compiler.

## Empirical domain check

The stage-1 native selfhost compiler was invoked on these representative completed-domain programs:

| Domain | Probe | Result under selfhost stage 1 |
| --- | --- | --- |
| Systems | `examples/systems_low_level.lan` | Not yet supported |
| Web | `examples/web_frontend.lan` | Not yet supported |
| Backend/Cloud | `examples/backend_poller.lan` | Not yet supported |
| ML/AI | `examples/ml_tensor.lan` | Not yet supported |
| Mobile | `examples/mobile_full.lan` | Not yet supported |
| Game | `examples/game_full.lan` | Not yet supported |
| DevOps | `examples/devops_script.lan` | Not yet supported |

These results do not invalidate the production implementations. They identify the gap between the production compiler and the self-hosted compiler.

## What is still C++ by design or legacy

### Production compiler

The authoritative production pipeline remains:

```text
Lanner source
  -> C++ lexer/parser
  -> C++ typechecker
  -> C++ native LLVM codegen
  -> clang/LLVM
```

### Driver and platform integration

The C++ driver still owns LLVM toolchain discovery, target selection, linker invocation, Web JS loader generation, Android project generation, iOS/Xcode project generation, and Game/CMake project generation.

### Legacy HIR

The old HIR implementation is fully C++ and is guarded by `LANNER_ENABLE_LEGACY_HIR`. It is not part of the default production `auto` path and should eventually be removable once compatibility/reference needs are retired.

### Runtime

The hosted runtime is **C**, not C++. That includes the networking, threading, filesystem, process, game, mobile, Web, tensor, and scripting runtime helpers. The runtime is therefore a separate native ABI/runtime layer, not a second C++ compiler implementation.

## Highest-priority bootstrap work

To make the compiler itself genuinely self-hosted, the next engineering phase should be:

1. Bring the Lanner lexer/token model to parity with `src/lexer/lexer.cpp`.
2. Bring the Lanner parser/AST model to parity with `src/parser/parser.cpp`.
3. Port the full ownership/permission/typechecker semantics from `src/sema/typechecker.cpp`.
4. Replace the compact bootstrap HIR with the production typed IR/data model and all required lowering passes.
5. Port the full native LLVM code generator, including built-in/runtime namespace lowering.
6. Port comptime evaluation and all compile-time declaration/constant rules.
7. Move driver/toolchain functionality behind Lanner libraries or a very small platform launcher.
8. Decide the final position of the legacy C++ HIR and delete it once its compatibility role is no longer needed.

The important architectural goal is **one authoritative Lanner semantics implementation**, not two subtly different memory/type systems.
