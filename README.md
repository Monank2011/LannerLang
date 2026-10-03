Stable

Stable 1.0.0

Stable is a general-purpose systems programming language designed for native performance with a statically checked ownership, borrowing, view, and region-based memory model.

Stable uses indentation-based syntax, compile-time checking, deterministic destruction, and LLVM as its native code-generation target.

The language is designed around a simple idea:

T            I own this value
&T           I can read this value temporarily
&mut T       I can mutate this value exclusively
View[T]      I can read this range temporarily
EditView[T]  I can mutate this range exclusively
Arena        this region owns these allocations

The programmer reasons about ownership and access. Stable infers borrow lifetimes and enforces the resulting safety rules without requiring explicit lifetime parameters.

---

Current status

Stable 1.0.0 is the current implemented language/compiler release.

The current release includes:

- indentation-based syntax
- static type checking
- deterministic ownership and destruction
- shared and exclusive references
- shared and exclusive range views
- fixed-size arrays
- dynamic owning arrays
- arena/region allocation
- structs
- enums
- Optional values
- Result values
- compile-time constants and expressions
- numeric casts
- arithmetic and bitwise expressions
- comparisons and logical operators
- "if" / "else if" / "else"
- "while"
- "for"
- "break"
- "continue"
- function calls and returns
- indexed and field assignment
- bounds checking
- borrow/escape checking
- native LLVM code generation
- LLVM toolchain selection
- diagnostic source locations and repair hints
- self-hosting infrastructure
- LLVM/HIR inspection and bootstrap tooling

Future features remain tracked separately in "ROADMAP.md".

---

Why Stable exists

Stable targets the space between high-level memory safety and low-level control.

The design goals are:

1. Native performance without a mandatory tracing garbage collector.
2. Ownership safety without explicit lifetime syntax.
3. Deterministic destruction.
4. Predictable data representation.
5. Cheap references and views.
6. Region allocation for short-lived object graphs.
7. Strong alias information for optimization.
8. A language surface that remains easier to reason about than explicit lifetime-heavy systems languages.
9. General-purpose use rather than a domain-specific focus.

Chess engines are one demanding workload used during Stable development and benchmarking, but Stable is intended for general systems programming.

---

Language overview

Indentation-based syntax

Stable uses indentation to delimit blocks.

main() i32:
    print("Hello, STABLE!")
    return 0

No brace syntax is required for ordinary blocks.

---

Primitive types

The current type system includes:

Integers

i8
i16
i32
i64
isize

u8
u16
u32
u64
usize

Floating point

f32
f64

Other built-in types

bool
string
void

Stable also has compiler-known aggregate/runtime types:

Arena
View[T]
EditView[T]

and language-level aggregate forms such as arrays, Optionals, Results, structs, and enums.

---

Literals

Stable supports:

- decimal integers
- hexadecimal integers
- binary integers
- floating-point literals
- string literals
- "true"
- "false"
- "none"

Examples:

a = 42
b = 0xFF
c = 0b101010
d = 3.5
e = true
f = "Stable"

Numeric context is propagated through unary operators, so explicit declarations such as:

x: f32 = -0.9
y: i64 = -918273645

preserve the intended numeric type.

---

Functions

Functions use:

functionName(parameters) returnType:
    ...

Example:

add(a: i32, b: i32) i32:
    return a + b

main() i32:
    return add(20, 22)

Functions may return scalars and supported aggregate values.

The production frontend resolves function declarations independently of their textual ordering, allowing forward references and mutual recursion in the supported compiler pipeline.

---

Variables and constants

Ordinary bindings:

x = 42

Explicit types:

x: i64 = 42

Constants:

const x = 42

Compile-time values:

comptime N = 2 + 1

"comptime" values are evaluated during compilation and may participate in compile-time-known contexts such as fixed-array sizes.

---

Control flow

"if"

main() i32:
    x = 10

    if x > 5:
        return 1

    return 0

"else if" / "else"

main() i32:
    x = 10

    if x < 5:
        return 1
    else if x == 10:
        return 2
    else:
        return 3

"while"

main() i32:
    x = 0

    while x < 10:
        x += 1

    return x

"for"

Stable supports array/view iteration:

main() i32:
    xs = [1, 2, 3, 4]
    sum: i32 = 0

    for x in xs:
        sum += x

    return sum

"break" and "continue" are supported.

---

Structs

Struct declarations:

Point[x: i32, y: i32]

main() i32:
    p = Point[x: 20, y: 22]
    return p.x + p.y

Struct fields participate in the ownership and borrow system.

Owning fields are transferred and destroyed according to the normal ownership rules.

---

Enums

Enums may contain integer-backed variants.

enum ErrorCode:
    Empty
    Invalid = 7

Enum members may be used as values:

return ErrorCode.Empty

Enums participate in "Result" and guard expressions.

---

Optional values

An Optional type is written:

T?

Example:

x: i32? = none

Stable supports contextual lifting from a payload into the corresponding Optional type where permitted.

---

Result values

Result values use:

Result[T, E]

with constructors:

Ok(value)
Err(error)

Example:

enum ErrCode:
    Empty

make(x: i32) Result[i32, ErrCode]:
    if x == 0:
        return Err(ErrCode.Empty)

    return Ok(x + 1)

main() i32:
    r = make(41)

    r is Ok(v) -> return v

    return 1

The "is Ok(...)" and "is Err(...)" forms provide typed guard bindings.

Owning Result payloads obey the same move and destruction rules as ordinary owning values.

---

Arrays

Stable has two array forms.

Fixed arrays

[N]T

Example:

comptime N = 3

main() i32:
    xs: [N]i32 = [10, 20, 30]
    return xs[2]

Fixed arrays support:

- construction
- indexing
- indexed assignment
- parameters
- returns
- struct fields
- mutable references
- bounds checking

Fixed arrays do not support "push()".

---

Dynamic arrays

[]T

Example:

main() i32:
    xs = [1, 2]
    xs.push(3)

    return xs[2]

Dynamic arrays support:

- ownership
- indexing
- indexed assignment
- ".len()"
- ".isEmpty()"
- ".push()"
- slicing
- shared views
- exclusive editing views
- passing and returning
- recursive ownership/destruction

---

References

Stable has two reference modes.

Shared reference

&T

A shared reference is read-only and non-owning.

Multiple shared references may coexist when their accesses do not conflict with mutation.

Exclusive reference

&mut T

An exclusive mutable reference provides mutation access while the referenced object is exclusively borrowed.

Example:

inc(v: &mut i64) void:
    v = 1

The reference does not become an owner.

---

Views

"View[T]"

A "View[T]" is a non-owning read-only range.

Conceptually:

pointer + length

Example:

main() i32:
    xs = [5, 8]
    view: View[i32] = xs[0:2]

    return view[0] + view[1]

"EditView[T]"

An "EditView[T]" is a non-owning exclusive mutable range.

It provides mutation access while preserving exclusivity over the affected range.

---

Slicing

Stable supports range slicing:

view = xs[0:4]

Views may be created from supported fixed and dynamic arrays.

Views can be resliced and indexed.

A view does not take ownership of its backing storage.

---

Arenas

Stable includes region-oriented allocation through "Arena".

Example:

Node[data: []i32]

main() i32:
    arena = Arena.create(256)
    nodes = [Node[data: [10]], Node[data: [20]]] in arena

    return nodes[0].data[0] + nodes[1].data[0]

Arena allocation is intended for:

- temporary object graphs
- parsers
- compiler data
- request-scoped data
- search structures
- workloads with bulk lifetime boundaries

Existing arena allocations are not relocated merely because the arena grows.

---

Compile-time computation

Stable supports compile-time integer, floating-point, string, and boolean expressions.

Example:

comptime N = 2 + 1

main() i32:
    xs: [N]i32 = [10, 20, 30]
    return xs[2]

The current evaluator includes compile-time safety checks such as:

- overflow detection
- division-by-zero detection
- expression folding
- boolean evaluation
- numeric evaluation
- string evaluation
- use in compile-time-known fixed-size contexts

---

Operators

The current expression system includes:

Arithmetic

+
-
*
/
%

Bitwise

&
|
^
~
<<
>>

Logical

&&
||
!

Comparisons

==
!=
<
>
<=
>=

Cast

x as i64

---

Built-in runtime surface

The current compiler-known runtime operations include:

readFile(string) -> []u8
writeStdout(string) -> void
writeRaw(string) -> void
writeIntRaw(i64) -> void
writeByteRaw(i64) -> void
print(value) -> void
printInt(i64) -> void
stringLen(string) -> usize
getEnv(string) -> string
Arena.create(usize) -> Arena

Collection methods currently include:

array.len()
array.isEmpty()
array.push(value)
view.len()
string.len()

"print()" currently supports:

- strings
- booleans
- integer values
- "f32"
- "f64"

Example:

main() i32:
    print("Hello")
    print(42)
    print(true)
    print(3.5)

    return 0

---

Memory safety model

Stable does not use tracing garbage collection as its core ownership mechanism.

It also does not silently turn ordinary values into reference-counted objects.

The core model is:

Owned value
    |
    +-- shared borrow --> &T
    |
    +-- exclusive borrow --> &mut T
    |
    +-- shared range --> View[T]
    |
    +-- exclusive range --> EditView[T]
    |
    +-- region ownership --> Arena

Borrow lifetimes are inferred by the compiler.

The source language does not require Rust-style lifetime parameters.

See:

"docs/MEMORY_MODEL.md"

for the complete memory-model specification.

---

Ownership

Owning values have one responsible owner at a time.

For non-copy values:

- assignment may transfer ownership
- function calls may transfer ownership
- returns may transfer ownership
- storing into an owning collection may transfer ownership

After a successful move, the old owner is no longer usable as an owner.

Stable rejects accidental use-after-move.

---

Destruction

Destruction is deterministic and ownership-driven.

The compiler tracks cleanup through lowering and generates the required destruction paths.

The current implementation includes recursive cleanup for:

- dynamic arrays
- structs
- Optional values
- Result values
- nested owning aggregates

The ownership model aims for:

one owner
    ->
zero or more ownership transfers
    ->
one final destruction

No owning resource should be destroyed twice.

---

Borrowing rules

The important rules are:

1. "&T" is read-only.
2. "&mut T" is exclusive and mutable.
3. "View[T]" is read-only.
4. "EditView[T]" is exclusive and mutable.
5. Conflicting borrows cannot overlap.
6. Owner mutation is blocked while a conflicting borrow is active.
7. Dynamic-array operations that may relocate storage are blocked while conflicting views/references are active.
8. A borrow cannot outlive its backing storage.
9. A view cannot outlive or outlive the validity of its backing storage.
10. Non-copy field/index projections do not silently become ownership moves.
11. Arena-backed references cannot escape the lifetime of their region.
12. Moved values cannot be used as though they still owned their previous storage.

---

Bounds safety

Array and view indexing is bounds checked unless the compiler can safely eliminate a check.

The compiler may use:

- known fixed lengths
- loop ranges
- dominating conditions
- view length invariants
- compile-time information

to remove checks when it can prove the access safe.

Failed runtime bounds checks trap deterministically.

---

Compiler architecture

The current production compiler is:

Stable source
   |
   v
Lexer
   |
   v
Parser / AST
   |
   v
Semantic checking
   |
   v
Comptime evaluation
   |
   v
Native LLVM lowering
   |
   v
LLVM IR
   |
   v
Clang/LLVM
   |
   v
Native executable

The repository also contains the legacy typed HIR/SSA infrastructure used for bootstrap/reference purposes.

The HIR path is optional and is enabled with:

-DSTABLE_ENABLE_LEGACY_HIR=ON

The production "auto" backend is native/direct LLVM and does not silently fall back to HIR.

---

LLVM toolchain support

Stable emits modern opaque-pointer LLVM IR and therefore requires LLVM/Clang 15 or newer.

The compiler does not hard-code one LLVM installation.

Select a toolchain with:

STABLE_CLANG=/path/to/clang

or:

LLVM_CC=/path/to/clang

Otherwise Stable uses:

clang

from the environment.

The repository includes:

tools/verify_llvm_compat.sh

for compatibility checking.

Floating-point constants are emitted using an exact LLVM-compatible representation so non-exact values do not depend on one particular LLVM release's decimal parser behavior.

---

Optimization

The compiler exposes:

-O0
-O1
-O2
-O3

The default driver optimization level is "-O2".

The typed-HIR optimizer currently includes safe transformations such as:

- scalar constant folding
- floating-point constant folding
- constant conditional simplification
- unreachable block removal
- dead pure-value elimination
- safe fixed-array bounds-check elimination
- conservative same-block scalar load forwarding

Ownership-bearing and reference-bearing aggregates are deliberately excluded from unsafe load forwarding.

The backend then passes the generated LLVM IR to Clang/LLVM for native optimization.

---

Diagnostics

Stable reports diagnostics with:

- file name
- line
- column
- highlighted source line
- error description
- repair-oriented help where available

Example:

file.st:4:7: error: cannot modify an owner while it is borrowed
  --> file.st:4:7
    |
  4 |     xs[0] = 9
    |       ^~~~~
    |
    = help: End the borrow before mutating the value, or use an exclusive
      EditView/&mut access when appropriate.

---

Self-hosting

Stable contains a Stable-written compiler frontend/backend track under:

selfhost/

The repository includes recursive bootstrap verification.

The native LLVM self-hosting verifier can repeatedly compile the Stable-written compiler through Stable itself and compare the resulting compiler artifacts.

The repository also retains the legacy HIR/C++ bootstrap route as compatibility infrastructure.

See:

"docs/BOOTSTRAP.md"

---

Build

Requirements:

- CMake 3.20 or newer
- C++17 compiler
- Clang/LLVM 15 or newer
- a Unix-like environment for the provided verification scripts

Build:

cmake -S . -B build -DSTABLE_BUILD_TESTS=ON
cmake --build build -j

Run tests:

ctest --test-dir build --output-on-failure

Build Stable with legacy HIR compatibility enabled:

cmake -S . -B build \
    -DSTABLE_BUILD_TESTS=ON \
    -DSTABLE_ENABLE_LEGACY_HIR=ON

---

Command-line usage

Build an executable:

stablec hello.st

Choose an output:

stablec hello.st -o hello

Run immediately:

stablec hello.st --run

Type-check only:

stablec hello.st --check

Emit LLVM:

stablec hello.st --emit-llvm

Emit LLVM to a file:

stablec hello.st --emit-llvm -o hello.ll

Select a backend:

stablec hello.st --backend=auto
stablec hello.st --backend=llvm

The legacy HIR backend requires a build configured with:

STABLE_ENABLE_LEGACY_HIR=ON

---

Project layout

src/
    lexer/            lexical analysis
    parser/           AST and parsing
    sema/             symbol and type checking
    comptime/         compile-time evaluator
    diagnostics/      diagnostic rendering
    codegen/          native LLVM backend
    ir/               legacy HIR/SSA backend

selfhost/
    lexer.st           Stable lexer
    parser.st          Stable parser
    compiler.st        Stable-written compiler

examples/
    language examples and regression programs

docs/
    language and architecture documentation

tools/
    bootstrap and compatibility verification scripts

---

Testing

Stable is tested at several levels:

- lexer/parser tests
- typechecker tests
- comptime tests
- integration tests
- diagnostics tests
- optimizer tests
- memory-model tests
- native self-host tests
- recursive bootstrap tests
- LLVM compatibility tests
- sanitizer tests
- benchmark-driven regression tests

The benchmark campaign also includes:

- basic scalar computation
- basic memory operations
- heavy memory allocation
- large dynamic arrays
- ownership churn
- arena growth
- nested ownership
- views and EditViews
- repeated lifecycle tests

The benchmark suites are intended to find bugs as well as measure speed.

---

Performance validation

Stable is designed for native performance.

Benchmarking is performed against equivalent C++ programs using the same LLVM/Clang toolchain where possible.

The project does not claim that Stable is universally faster than C++ or Rust based on one benchmark.

Runtime speed, generated code, correctness, memory behavior, and compiler behavior all matter.

---

Current release boundaries

Stable 1.0.0 is not intended to claim every future language feature.

Items such as:

- full general-purpose generic-function monomorphization
- exhaustive pattern matching
- modules/package management
- safe FFI
- concurrency and channels
- atomics
- SIMD/vector abstractions
- comptime functions/reflection
- PGO
- debugger/LSP tooling
- cross-compilation and linker integration

remain future roadmap work unless explicitly listed as implemented in the current release documentation.

---

License

Stable's source code should be published under the project's chosen open-source license.

Add the corresponding "LICENSE" file at repository root before release.

---

Contributing

See:

"CONTRIBUTING.md"

Security

See:

"SECURITY.md"

Roadmap

See:

"ROADMAP.md"

---

First program

main() i32:
    print("Hello, STABLE!")
    return 0

Build it:

stablec hello.st

Run it:

./hello

Welcome to Stable.
