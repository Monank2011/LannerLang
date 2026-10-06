# Lanner engine runtime

Lanner 1.0 now exposes the small runtime surface needed by native engines while keeping the compiler in charge of types, ownership, and lowering.

## Interactive stdin

`Stdin.readLine()` blocks until a line is available and returns it without the trailing CR/LF. `Stdin.hasInput()` is a non-blocking readiness check suitable for a search loop that must also watch for `stop` or `quit`.

```lanner
while running:
    line: string = Stdin.readLine()
    if line.equals("quit"):
        running = false
```

String helpers useful for protocol parsing are available as methods:

```lanner
line.startsWith("go")
line.equals("isready")
value: u64 = line.parseU64At(3)
byte: u8 = line[0]
```

## Static/global storage

Use `static` for module-wide storage. Initializers must be compile-time constants, so lookup tables have deterministic program startup and no hidden initialization function.

```lanner
static PIECE_VALUES: [8]i32 = [100, 320, 330, 500, 900, 20000, 0, 0]
static mut Nodes: Atomic[u64] = 0
```

`static` is read-only by default. `static mut` permits writes. Indexed writes use the normal Lanner bounds checks and ownership rules.

## C FFI

Declare a C ABI symbol with `extern` and link any extra object/library through `--link`:

```lanner
extern lanner_ffi_add(a: i32, b: i32) i32

main() i32:
    return lanner_ffi_add(40, 2)
```

Compiler-provided declarations are emitted once, so FFI declarations may also name common C runtime functions such as `strlen` without creating duplicate LLVM declarations.

Examples:

```text
lanner examples/ffi.st --link path/to/object.o -o ffi_demo
lanner examples/ffi.st --link -lm -o ffi_demo
```

The extra link arguments are passed directly to the final Clang link step.

## CPU intrinsics and SIMD

The `Cpu` namespace has explicit, opt-in engine primitives:

- `Cpu.hasAvx2()`, `Cpu.hasAvx512()`, `Cpu.hasSse42()`, `Cpu.hasBmi2()`, `Cpu.hasPopcnt()`
- `Cpu.rdtsc()`
- `Cpu.popcount()`, `Cpu.ctz()`, `Cpu.clz()`, `Cpu.bswap()`
- `Cpu.pext()`, `Cpu.pdep()` for BMI2 targets
- `Cpu.prefetch(&value)`
- `Cpu.loadV128/256/512(&u64)` and matching stores
- `Cpu.zeroV128/256/512()`

Lanner vectors are fixed-width value types:

```lanner
v: v256 = Cpu.loadV256(&TABLE[0])
w: v256 = v.xor(v)
first: u64 = w.extractU64(0)
```

Feature probes are runtime checks. Code that calls an ISA-specific intrinsic should still gate the path with the corresponding `Cpu.has*()` query when portability matters.

## Threads and atomics

Lanner provides a small native-thread API rather than hidden scheduling:

```lanner
worker() void:
    Nodes.fetchAdd(1)

main() i32:
    t: Thread = Thread.spawn(worker)
    t.join()
    return Nodes.load() as i32
```

`Atomic[T]` supports:

- `Atomic.new(value)` in a contextual `Atomic[T]` expression
- `load()` and `store(value)`
- `fetchAdd(value)` and `fetchSub(value)`
- `compareExchange(expected, desired)`

`Thread.hardwareConcurrency()` reports the host's available hardware-thread count, and `Thread.yield()` yields the current OS thread.

## Engine clocks

The clock is monotonic, not wall-clock based:

```lanner
deadline: u64 = Clock.deadlineAfterNanos(5000000)
while !Clock.expired(deadline):
    searchIteration()
```

Available operations:

- `Clock.monotonicNanos()`
- `Clock.deadlineAfterNanos(duration)`
- `Clock.remainingNanos(deadline)`
- `Clock.expired(deadline)`
- `Clock.sleepNanos(duration)`

This is sufficient for iterative-deepening engines, aspiration/search budgets, and polling UCI `stop` while a worker thread is active.

## Runtime selection

The native LLVM backend automatically links `src/runtime/lanner_runtime.c`. Set `LANNER_RUNTIME` to override the runtime C source used by `lanner`, which is useful for packaged installs and custom runtime builds.

```text
LANNER_RUNTIME=/path/to/lanner_runtime.c lanner engine.st -O3 -o engine
```

The language primitives stay explicit in the source language. There is no hidden garbage collector, reference counting, or implicit thread pool behind this interface.
