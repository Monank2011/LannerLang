# **LannerLang 3.0.0**

## **What is new in Lanner 3.0.0**

### Compound assignment

```lanner
x += 5
mask &= 0xFF
state <<= 1
board.nodes += 1
table[(hash >> 4) & 1023] ^= key
```

All of `+= -= *= /= %= &= |= ^= <<= >>=` work on variables, struct fields and indexed elements.
`place op= value` means `place = place op value`, and the place is evaluated twice, so a place that contains a
function call is rejected with a diagnostic instead of silently calling it twice.

### Range loops

```lanner
for i in 0..n:        # 0, 1, ... n-1  (exclusive)
    total += i
for i in 1..=10:      # 1, 2, ... 10   (inclusive)
    total += i
```

- A literal bound adopts the type of the other bound, so `for i in 0..n` with `n: usize` makes `i` a `usize`.
- Both bounds must have the same integer type. Bounds are evaluated once, before the loop starts.
- The loop variable is read-only inside the body. `continue` and `break` behave as in `while`.
- `0..=255` on a `u8` terminates correctly; inclusive loops stop *at* the last value instead of wrapping.
- The counter increment carries `nuw`/`nsw`, giving LLVM an exact trip count for unrolling and vectorization.

### Smaller conveniences

- `elif cond:` is shorthand for `else if cond:`.
- Digit separators in numeric literals: `1_000_000`, `0xFF_FF`, `0b1010_0101`, `3.141_592`.

### Performance and tooling

- Dynamic-array growth (capacity doubling, overflow checks, arena/heap allocation, copy) is one shared
  `cold noinline` helper instead of being duplicated at every `push`. Generated IR is smaller, hot loops are tighter,
  and the helper takes the array header **by value** so the header never escapes and stays in registers.
- Whole-program hosted executables are emitted with `internal` linkage, which lets LLVM do interprocedural
  optimization across all user functions. Object, library, web and freestanding builds, and any build using
  `--link`, keep external symbols. Use `--keep-symbols` to opt out explicitly.

> **A native systems programming language with compile-time ownership safety, deterministic memory management, and LLVM code generation.**
> **Created and developed by Monank Gohil, who began the project at age 15.**

Lanner is a general-purpose systems programming language built around a simple goal: make **native, predictable performance** compatible with a **strong static memory-safety model** that remains practical to learn.

Lanner uses **indentation-based syntax**, **static typing**, **ownership and borrowing**, **non-owning views**, **region/arena allocation**, and **LLVM** for native code generation.

The native engine stack includes interactive stdin, compile-time static/global tables, explicit C FFI, CPU/SIMD intrinsics, OS threads and atomics, a monotonic engine clock, and a systems/low-level layer for raw pointers, allocation, inline assembly, volatile memory, ABI/layout control, and freestanding cross-target builds. See [`docs/ENGINE_RUNTIME.md`](docs/ENGINE_RUNTIME.md), [`docs/SYSTEMS.md`](docs/SYSTEMS.md), and [`examples/uci_engine.lan`](examples/uci_engine.lan).

The hosted backend/cloud runtime adds TCP/UDP sockets, scalable readiness polling, synchronization primitives, child processes, HTTP/1.1 client operations, JSON serialization/validation, and owned byte buffers. See [`docs/BACKEND_CLOUD.md`](docs/BACKEND_CLOUD.md) and the `examples/backend_*.lan` programs.

The DevOps/scripting runtime adds process arguments, environment management, filesystem and path APIs, lightweight regex, shell command/output helpers, executable lookup, and compiled-script execution through `lanner --script ... -- args`. See [`docs/DEVOPS_SCRIPTING.md`](docs/DEVOPS_SCRIPTING.md) and [`examples/devops_script.lan`](examples/devops_script.lan).

The Version 1 native engine readiness audit now verifies that Lanner can compile and execute representative chess bitboard/search code, the UCI runtime, and a representative 45,192 → 16 → 32 → 1 quantized NNUE inference path entirely from Lanner source. See [`docs/CHESS_NNUE_AUDIT.md`](docs/CHESS_NNUE_AUDIT.md), [`examples/chess_engine_kernel_audit.lan`](examples/chess_engine_kernel_audit.lan), and [`examples/nnue_host_audit.lan`](examples/nnue_host_audit.lan).

The compiler is designed so the source-level memory rules are authoritative. Backend lowering must preserve those rules rather than inventing a separate safety convention.

Full self-hosting is not yet claimed: the recursive Lanner-written bootstrap covers a supported subset, while the complete production compiler and platform driver remain in C++. See [`docs/BOOTSTRAP_AUDIT.md`](docs/BOOTSTRAP_AUDIT.md).

---

## **Project author**

Lanner was created and is developed by **Monank Gohil**, who began developing the language at age **15**. See [`AUTHORS.md`](AUTHORS.md).

## **Lanner at a glance**

```text
T            Own this value.
&T           Borrow this value for read-only access.
&mut T       Borrow this value for exclusive mutable access.
View[T]      Borrow a range for read-only access.
EditView[T]  Borrow a range for exclusive mutable access.
Arena        Own a region containing allocations.
```

Lanner's borrow lifetimes are **inferred**. Ordinary source code does not require explicit lifetime parameters.

The core model does **not** require tracing garbage collection and does **not** silently introduce reference counting for ordinary values.

[![Release](https://img.shields.io/github/v/release/Monank2011/LannerLang?display_name=tag&sort=semver)](https://github.com/Monank2011/LannerLang/releases/tag/v3.0.0)
[![License](https://img.shields.io/github/license/Monank2011/LannerLang)](LICENSE)
[![Build](https://img.shields.io/badge/build-CMake%20%2B%20C%2B%2B17-blue)](CMakeLists.txt)

> **Lanner 3.0.0 is the current major release.** Download a ready-to-run compiler below, or build the compiler from source.

### Release downloads

- **[Linux x86_64 compiler](https://github.com/Monank2011/LannerLang/releases/download/v3.0.0/lanner-3.0.0-linux-x86_64.tar.gz)**
- **[Windows x86_64 compiler](https://github.com/Monank2011/LannerLang/releases/download/v3.0.0/lanner-3.0.0-windows-x86_64.zip)**
- **[Source code (this repository)](https://github.com/Monank2011/LannerLang/tree/v3.0.0)**

Each binary package includes the `lanner` compiler, runtime support files, examples, and an installer that downloads official LLVM/Clang **23.1.2** into a Lanner-owned side-by-side directory. The installer verifies the download checksum and does not replace a system LLVM installation. SHA-256 checksums are published with the GitHub release. See [`docs/release/README.md`](docs/release/README.md).

For convenience, see the [installation guide](docs/INSTALL.md) for a `pip` command that fetches the latest release and a `g++` source-build command.

New users can start with the [**Lanner 3.0.0 Handbook**](Lanner_handbook.md), a practical guide to the language, ownership model, syntax, runtime, and V1 capabilities.


---

## **Version 3.0.0**

Lanner 3.0.0 is the current published implementation of the Lanner compiler and language surface.

The release includes:

- **Indentation-based blocks**
- **Static type checking**
- **Deterministic ownership and destruction**
- **Move/use-after-move checking**
- **Shared `&T` and exclusive `&mut T` references**
- **`View[T]` and `EditView[T]`**
- **Fixed `[N]T` and dynamic `[]T` arrays**
- **Arena/region allocation**
- **Structs and enums**
- **Optional values**
- **`Result[T, E]` values**
- **Compile-time expressions and fixed-size specialization**
- **Integer, floating-point, boolean, and string values**
- **Arithmetic, logical, comparison, and bitwise operators**
- **`if`, `else if`, `else`, `while`, and `for`**
- **`break` and `continue`**
- **Function calls, returns, forward references, and mutual recursion**
- **Bounds checking and bounds-check elimination when proven safe**
- **Native LLVM code generation**
- **LLVM toolchain selection**
- **Source-aware diagnostics**
- **Native self-hosting infrastructure**
- **Recursive bootstrap verification**
- **LLVM/HIR inspection and compatibility tooling**
- **Explicit unsafe raw pointers, manual/aligned/stack allocation, and volatile/unaligned memory operations**
- **Inline assembly, CPU feature control, SIMD values, atomics, and OS threads**
- **C-compatible scalar/pointer/function-pointer FFI and external globals**
- **Freestanding/no-runtime builds, target triples, cross-target objects, linker scripts, and custom link options**
- **Systems regression coverage across x86-64, i386, and AArch64 object generation**
- **Hosted backend/cloud runtime with TCP/UDP sockets, epoll-backed polling on Linux, synchronization primitives, process handles, HTTP/1.1 GET/POST, JSON primitives, and growable byte buffers**
- **Backend integration coverage for networking, synchronization, process management, HTTP, JSON, and high-fanout poller registration**
- **DevOps/scripting runtime with arguments, environment variables, filesystem/path operations, regex, shell execution/output, and compiled script mode**
- **Native ML/AI tensor runtime with f32/f64 multidimensional tensors, slicing/reshape/transpose, reductions, matmul, activations, softmax, convolution, explicit autodiff tapes, and accelerator/BLAS discovery**

Future expansion is tracked separately in **`ROADMAP.md`**.

---

## **Why Lanner exists**

Lanner targets the part of systems programming where programmers want both:

- **Low-level control and native performance**, and
- **Strong compile-time memory guarantees**.

The design deliberately avoids making safety depend on a mandatory tracing collector or pervasive hidden reference counting.

The primary design goals are:

1. **Native performance**.
2. **Deterministic destruction**.
3. **Ownership safety without explicit lifetime syntax**.
4. **Cheap references and views**.
5. **Predictable representations and costs**.
6. **Region allocation for bulk-lifetime workloads**.
7. **Strong alias information usable by optimizers**.
8. **A memory model that is practical to learn**.
9. **A general-purpose language rather than a domain-specific language**.

---

## **Hello, Lanner**

```lanner
main() i32:
    print("Hello, LANNER!")
    return 0
```

Build:

```sh
lanner hello.lan
```

Run:

```sh
./hello
```

---

## **Type system**

### **Integers**

```text
i8   i16   i32   i64   isize
u8   u16   u32   u64   usize
```

### **Floating point**

```text
f32
f64
```

### **Other built-in types**

```text
bool
string
void
```

### **Compound types**

```text
[N]T
[]T
T?
Result[T, E]
&T
&mut T
View[T]
EditView[T]
Arena
```

Structs and enums provide user-defined aggregate and variant types.

---

## **Ownership and moves**

An owning value has one responsible owner at a time.

For non-copy values, ownership may move through:

- initialization
- assignment
- function calls
- returns
- insertion into owning collections
- aggregate construction

After a valid move, the old owner cannot continue to be used as though it owns the resource.

This prevents common **use-after-move** and **double-destruction** errors.

Field and index projections do not silently turn into owning extraction operations.

---

## **Borrowing**

### **Shared borrow: `&T`**

A shared reference is **non-owning** and **read-only**.

Multiple compatible shared borrows may coexist.

### **Exclusive borrow: `&mut T`**

An exclusive reference is **non-owning** and grants mutation access.

While an exclusive borrow is active, conflicting accesses are rejected.

Example:

```lanner
inc(v: &mut i64) void:
    v = v + 1
```

Lanner automatically determines when the borrow is no longer needed.

---

## **Views**

`View[T]` is a **shared, non-owning range view**.

`EditView[T]` is an **exclusive, non-owning mutable range view**.

Conceptually, both are small range descriptors containing storage information and a length. They do not own the backing storage.

Dynamic-array operations that could relocate storage are prevented while conflicting references or views are still live.

---

## **Arrays**

### **Fixed arrays**

```lanner
comptime N = 4
xs: [N]i32 = [1, 2, 3, 4]
```

Fixed arrays support indexed access and mutation and participate in ownership/borrow checking.

### **Dynamic arrays**

```lanner
xs: []u64 = []u64.new()
xs.push(4)
print(xs.len())
```

Allocation-sensitive code can preallocate and tune growth:

```lanner
xs: []u64 = []u64.with_capacity(1024)
xs.reserve(4096)
print(xs.capacity())
```

Dynamic arrays support ownership, indexed access, mutation, length/capacity queries, emptiness checks, explicit reservation, growth, slicing, views, and deterministic cleanup. A typed empty array can also be written directly as `[]u64`; a dummy seed element is not required.

---

## **Bounds safety**

Array and view indexing is checked unless the compiler can **prove the access safe**.

The implementation can use fixed sizes, loop ranges, dominating conditions, view lengths, and compile-time information to eliminate checks when safe.

Invalid runtime indexing traps instead of silently touching unrelated memory.

---

## **Arenas and regions**

`Arena` provides **region-oriented allocation** for workloads where many objects share a lifetime boundary.

Typical uses include:

- parsers
- compiler data structures
- temporary graphs
- request-scoped data
- search structures
- other bulk-lifetime workloads

Arena growth does not relocate existing allocations merely because another arena chunk is required.

Arena-backed values and references remain subject to **escape and lifetime checking**.

---

## **Optionals**

```lanner
value: i32? = none
```

Optional values participate in normal ownership and destruction rules when their payload is owning.

---

## **Results**

```lanner
enum ErrorCode:
    Empty

make(x: i32) Result[i32, ErrorCode]:
    if x == 0:
        return Err(ErrorCode.Empty)
    return Ok(x + 1)
```

Supported forms include `Ok(...)`, `Err(...)`, and typed `is Ok(...)` / `is Err(...)` guards.

---

## **Comptime**

Lanner evaluates supported compile-time expressions during compilation.

Current compile-time values include **integer**, **floating-point**, **boolean**, and **string** expressions.

Example:

```lanner
comptime N = 2 + 2
xs: [N]i32 = [10, 20, 30, 40]
```

The compile-time evaluator includes checks such as **overflow detection** and **division-by-zero detection**.

---

## **Control flow**

Lanner provides:

```text
if / else if / else
while
for
break
continue
```

`for` iteration supports arrays and supported views, with loop borrows participating in the same safety rules as ordinary borrows.

---

## **Operators**

Arithmetic:

```text
+  -  *  /  %
```

Bitwise:

```text
&  |  ^  ~  <<  >>
```

Logical:

```text
&&  ||  !
```

Comparison:

```text
==  !=  <  >  <=  >=
```

Explicit conversion:

```lanner
x as i64
```

Lanner supports decimal, hexadecimal, and binary integer literals plus floating-point literals.

---

## **Diagnostics**

Lanner diagnostics include **file, line, column, highlighted source**, and repair-oriented help when available.

Example:

```text
file.lan:4:7: error: cannot modify an owner while it is borrowed
  --> file.lan:4:7
    |
  4 |     xs[0] = 9
    |       ^~~~~
    |
    = help: End the borrow before mutating the value, or use an exclusive
      EditView/&mut access when appropriate.
```

---

## **LLVM backend**

The production compiler lowers Lanner to **LLVM IR**, then invokes the selected LLVM/Clang toolchain for native code generation.

Toolchain selection:

```sh
LANNER_CLANG=/path/to/clang lanner hello.lan
```

or:

```sh
LLVM_CC=/path/to/clang lanner hello.lan
```

If neither is set, the compiler uses `clang` from the environment.

Lanner emits modern **opaque-pointer LLVM IR** and targets the LLVM 15+ IR family.

Run compatibility checks with:

```sh
tools/verify_llvm_compat.sh
```

---

## **Optimization**

Compiler optimization levels:

```text
-O0  -O1  -O2  -O3
```

The default driver level is `-O2`.

The compiler has safe constant folding, conditional simplification, unreachable-block removal, dead pure-value elimination, safe fixed-array bounds-check elimination, and conservative scalar load forwarding in the relevant IR path.

The LLVM backend then performs target-level optimization.

---

## **Self-hosting**

Lanner contains a Lanner-written compiler track and recursive bootstrap verification.

The native self-hosting path can compile the Lanner-written compiler through the Lanner compiler itself and compare resulting stages.

The bootstrap infrastructure verifies a **byte-identical stage-2/stage-3 fixed point** for the validated native path.

---

## **Memory safety**

Lanner's core memory invariants include:

- **no use-after-move**
- **no double destruction**
- **no dangling borrow**
- **no conflicting exclusive aliases**
- **no mutation through shared access**
- **no invalidating dynamic-array relocation while a conflicting borrow is live**
- **no invalid arena/reference escape**
- **bounds-safe indexing**
- **deterministic destruction**

See **`Memory_Model.md`** for the complete model.

---

## **Validation**

Lanner has been exercised through:

- compiler regression tests
- native self-hosting tests
- recursive bootstrap checks
- scalar computation benchmarks
- basic memory benchmarks
- heavy memory stress
- AddressSanitizer
- UndefinedBehaviorSanitizer
- LeakSanitizer
- LLVM compatibility tests

Heavy-memory testing reached roughly **1.37 GiB peak RSS** in the largest completed red-zone workload without sanitizer findings in the tested workloads.

These tests are evidence from exercised programs, not a mathematical proof of compiler infallibility.

See **`Benchmarks.md`** for the methodology and results.

---

## **Build from source**

Requirements:

- **CMake 3.20+**
- **C++17 compiler**
- **Clang/LLVM 15+**

```sh
cmake -S . -B build -DLANNER_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Legacy HIR compatibility:

```sh
cmake -S . -B build \
    -DLANNER_BUILD_TESTS=ON \
    -DLANNER_ENABLE_LEGACY_HIR=ON
```

---

## **Command-line interface**

```sh
lanner program.lan
lanner program.lan -o program
lanner program.lan --run
lanner program.lan --check
lanner program.lan --emit-llvm
```

Backend selection:

```sh
lanner program.lan --backend=auto
lanner program.lan --backend=llvm
lanner program.lan --backend=hir
```

`auto` selects the production native LLVM path. `hir` is the optional legacy/bootstrap path.

---

## **Project layout**

```text
src/          Compiler implementation
selfhost/     Lanner-written compiler track
examples/     Language and regression examples
docs/         Detailed technical documentation
tools/        Bootstrap and compatibility tooling
```

---

## **Documentation**

- **`Language.md`**: language reference
- **`Memory_Model.md`**: complete ownership/borrowing/region model
- **`Bootstrap.md`**: self-hosting and recursive bootstrap
- **`Benchmarks.md`**: benchmark methodology and validation
- **`ChangeLog.md`**: release history
- **`ROADMAP.md`**: future features
- **`CONTRIBUTING.md`**: contribution guide
- **`SECURITY.md`**: security reporting policy

---

## **Release scope**

Lanner 3.0.0 is a complete release of the **currently implemented language/compiler surface**, including native systems programming, browser WebAssembly, hosted backend/cloud primitives, native ML/AI compute, mobile targets, and game-development APIs.

Future expansion focuses on broader language/ecosystem features such as general-purpose generics, richer pattern matching, modules/packages, language-level async/await, advanced GPU libraries, PGO, and broader tooling.

---

## **License**

See **`LICENSE`**.


## Building on Linux and Windows

Lanner 3.0.0 is designed to build on both Linux and Windows using CMake 3.20+ and a C++17 compiler. The production backend uses an external LLVM/Clang toolchain.

### Linux

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### Windows

With Visual Studio or another C++17-capable toolchain:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

Install a modern LLVM/Clang toolchain and ensure `clang` is on `PATH`, or configure `LANNER_CLANG_EXECUTABLE` at CMake configure time. Lanner accepts LLVM/Clang 15 or newer because its generated IR uses opaque pointers.

The compiler driver contains platform-specific process, quoting, executable-name, and temporary-file handling so the same Lanner source tree can build and run on POSIX systems and Windows.

## Web Frontend

Lanner supports browser-oriented WebAssembly builds with `lanner --web`; see `docs/WEB.md`.

## Mobile targets

Lanner 3.0.0 includes native Android and iOS target support, mobile project generation, JNI/Objective-C bridge scaffolding, native UI shells, and a platform-neutral `Mobile` API for screen metrics, safe areas, storage paths, clipboard, URLs, haptics, permissions, and hardware capability queries. See `docs/Mobile.md`.

## Game development

Lanner 3.0.0 includes a native SDL2-backed `Game` runtime for windows, events, input, 2D rendering, textures, audio and frame timing, plus a `Graphics` layer for OpenGL and dynamically loaded native GPU APIs. `lanner --game-project` generates a portable CMake game project. See `docs/GAME_DEVELOPMENT.md`.
