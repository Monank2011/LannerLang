# Stable Language Design

Stable is a general-purpose systems programming language. Chess engines are one demanding workload used to stress-test the language, not the language's domain.

## Design goals

1. Native performance: generated code should have no mandatory garbage collector, no hidden heap allocation, and predictable data layouts. C++/Rust-class performance is the target; actual superiority must be demonstrated with reproducible benchmarks.
2. Memory safety by default: invalid references, use-after-move, out-of-bounds indexing, data races, and lifetime escapes should be compiler errors or defined traps when proven impossible to eliminate.
3. Low cognitive load: ordinary code should not require explicit lifetime parameters or borrow-checker choreography.
4. Explicit costs: allocation, synchronization, copying, and dynamic dispatch should be visible in source or in an explicit type/effect.
5. Generality: networking, game engines, compilers, embedded software, scientific code, applications, and high-performance compute should all be first-class targets.

## Memory model: Owned + View + Region

### Owned values

`T` is an owned value by default. Non-copy values move when ownership is transferred. Small scalar types, references, and views are cheap copy types.

### References

`&T` is a short-lived non-owning reference. The compiler infers its valid scope and rejects escapes.

### Views and editing views

`View[T]` is a copyable, non-owning, read-only slice represented as pointer + length. Slicing uses `a[lo:hi]`.

`EditView[T]` is a non-owning, exclusive, mutable slice. It is move-only. The compiler prevents another shared or mutable borrow of the owner while an `EditView` is alive.

References follow the same rule: `&T` is shared/read-only and `&mut T` is exclusive/mutable. The compiler infers the borrow lifetime lexically in the first safety pass and will later shorten it with flow-sensitive last-use analysis. Users do not write lifetime annotations in normal Stable code.

The intended mental model is:

- values own data
- `&T` borrows one value for reading
- `&mut T` temporarily edits one value
- `View[T]` reads a sequence
- `EditView[T]` edits a sequence exclusively

### Regions

`Arena`/regions provide bulk allocation. Objects allocated in a region share its lifetime. Region-backed values cannot escape their region unless explicitly copied into an outer owner. This makes temporary graphs, request-scoped data, parsers, and search stacks cheap without a tracing GC.

### Shared ownership

Reference counting is opt-in (`Shared[T]` and eventually `SharedSync[T]`). Stable will never silently turn an ordinary local into an atomic reference-counted object.

### Unsafe boundary

Raw pointers and operations that bypass safety checks belong behind an explicit `unsafe` boundary. The safe language should remain expressive enough that unsafe code is the exception, not the normal escape hatch.

## Performance principles

- monomorphize generics where profitable
- specialize comptime-known sizes and constants
- lower to a typed HIR/SSA form before LLVM
- eliminate bounds checks using range analysis
- preserve aliasing information in IR so LLVM can optimize safely
- provide SIMD/vector types and explicit intrinsics
- expose alignment/layout controls
- provide `no_alloc` and similar effects later
- support profile-guided optimization later
- keep compile-time work cacheable and incremental

## Features intended to differentiate Stable

### 1. Inferred borrowing instead of explicit lifetime syntax

Most users should write `View[T]` and ordinary references without naming lifetimes. The compiler derives the ownership graph from lexical scope and function signatures.

### 2. Region-first allocation

Instead of teaching every developer multiple smart-pointer forms, regions make a common high-performance lifetime pattern natural: create a region, build temporary data, release the whole region at the end.

### 3. Safe views as a primitive

Slices/views are first-class values, not library wrappers with hidden ownership rules. Their pointer+length representation should be ABI-stable and zero-cost.

### 4. Explicit cost surface

Stable should make the expensive things obvious: allocation, atomic sharing, locks, copies of large values, dynamic dispatch, and FFI transitions.

### 5. Comptime as normal language machinery

Compile-time computation should be able to specialize algorithms, generate lookup tables, validate invariants, and expose reflection without requiring a second metaprogramming language.

### 6. Productive diagnostics

Ownership and type errors should explain the owner, the borrow/view origin, the attempted escape, and a concrete repair path instead of presenting only an abstract constraint failure.

## General-purpose roadmap

- scalar types, expressions, control flow
- structs, enums, fixed arrays
- views/references and region allocation
- dynamic arrays and canonical `Option`/`Result`
- typed HIR/SSA
- monomorphized generics
- pattern matching and exhaustive checking
- deterministic cleanup (`defer`)
- modules and package management
- safe FFI
- concurrency, atomics, and channels
- SIMD/vector types
- comptime functions and reflection
- optimizer passes and PGO
- reproducible cross-compilation and linker integration

## Benchmark policy

Stable must not claim to beat C++ or Rust because a single microbenchmark looks good. The project should eventually ship reproducible benchmark suites for:

- compiler throughput
- integer/bit manipulation
- allocations and region workloads
- collections
- parsing/serialization
- numerical kernels
- concurrency
- real applications such as Guy

Correctness, generated assembly, binary size, and compile time are measured alongside runtime speed.
