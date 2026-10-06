# **Lanner Roadmap**

Lanner 1.0.0 is the current implemented release. Systems/low-level, Web frontend, Backend/Cloud foundation, ML/AI core, Mobile targets, and Game Development core are implemented and regression-tested. The remaining items below are future language/ecosystem expansion rather than missing foundations for those domains.

## **Language**

- **General user-defined generics**
- **Generic monomorphization and specialization**
- **General `match` / exhaustive pattern matching**
- **Destructuring patterns**
- **Modules and namespaces**
- **Package management**
- **Separate compilation**

## **Compile-time system**

- **Comptime functions**
- **Comptime loops and data generation**
- **Compile-time reflection**
- **Generated-code facilities**

## **Concurrency**

- **Channels**
- **Language-level async/await and task scheduling**
- **Expanded race-aware semantics**

## **Performance**

- **Deeper alias/range analysis**
- **Stronger bounds-check elimination**
- **Compiler-driven auto-vectorization**
- **Higher-level portable SIMD abstractions**
- **Profile-guided optimization**
- **Link-time optimization integration**
- **Incremental compilation**
- **Compilation caching**

## **Tooling**

- **Language server**
- **Debugger integration**
- **Formatter**
- **Linter**
- **Documentation generator**
- **Package manager tooling**
- **Expanded editor integrations**

## **Interoperability**

- **Higher-level safe FFI wrappers**
- **Additional ABI/calling-convention coverage**
- **Platform-specific library bindings**

## **Standard library**

Implemented hosted areas:

- **networking and readiness polling**
- **process management**
- **serialization primitives**
- **threading, atomics, and synchronization**

Still planned for later standard-library expansion:

- **collections**
- **hashing**
- **time/date**
- **database, TLS, and gRPC client libraries**

Future features should preserve Lanner's core principles:

> **safe by default, native performance, explicit costs, predictable ownership, and low cognitive overhead.**


## Web Frontend status

Complete for the browser-oriented wasm32 core: `lanner --web`, generated ES-module loader, JS FFI imports, exported Lanner functions, linear-memory services, DOM/event/timer/animation/microtask/Fetch primitives, browser-safe runtime boundaries, and regression tests.

Remaining browser-ecosystem work is intentionally separated from the core target: first-class WebGPU bindings, WASI/component-model support, and language-level `async`/`await` lowering.


## Backend / Cloud status

The hosted backend/runtime foundation is complete for Step 3: TCP/UDP sockets, Linux epoll-backed readiness polling with portable fallbacks, nonblocking I/O, synchronization, child processes, HTTP/1.1 GET/POST, JSON validation/serialization, and growable buffers are implemented and regression-tested. Higher-level TLS, database, gRPC, and cloud-provider packages remain ecosystem work rather than core runtime primitives.

## ML / AI completion

The 1.0 systems/web/backend tree now includes the native ML runtime. Remaining work belongs to optional ecosystem libraries and vendor bindings rather than missing core tensor/autodiff primitives.

## Mobile status

The 1.0 compiler supports native Android/iOS target triples, ARM64 object generation, generated Android NDK/CMake projects, generated Xcode/SwiftUI projects, and a native JNI/Objective-C bridge.


## DevOps / scripting status

The Step 7 core is complete: process arguments, environment variables, filesystem operations, path utilities, regex, shell execution/output, command lookup, and compiled script mode (`lanner --script file.st -- ...`) are implemented and regression-tested.

Remaining DevOps ecosystem work is intentionally outside the core runtime: package management, richer archive/configuration formats, terminal UI libraries, service-manager/cloud-provider integrations, and shell-specific convenience packages.

## Bootstrap status

The native Lanner-written compiler has a recursive stage-1 -> stage-2 -> stage-3 fixed point for its supported bootstrap subset. It is **not yet feature-complete with the production compiler**. Full typed parsing, semantic analysis, language services, platform project generation, and the complete native LLVM surface still depend on the C++ production compiler. See [`docs/BOOTSTRAP_AUDIT.md`](docs/BOOTSTRAP_AUDIT.md).
