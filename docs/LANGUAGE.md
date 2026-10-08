# **Lanner 3.0.0 Language Reference**

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

This document describes the currently implemented Lanner language surface.

## **1. Syntax**

Lanner uses **indentation-based blocks**.

```lanner
main() i32:
    print("Hello, LANNER!")
    return 0
```

## **2. Primitive types**

```text
i8 i16 i32 i64 isize
u8 u16 u32 u64 usize
f32 f64
bool string void
```

## **3. Compound types**

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

## **4. Functions**

```lanner
add(a: i32, b: i32) i32:
    return a + b
```

Function declarations are resolved independently of textual ordering in the supported compiler path, allowing **forward references and mutual recursion**.

## **5. Values**

Inference:

```lanner
x = 42
```

Explicit typing:

```lanner
x: i64 = 42
```

Constants:

```lanner
const x = 42
```

Compile-time values:

```lanner
comptime N = 4
```

## **6. Literals**

Supported forms include:

```text
42
0x2A
0b101010
3.5
-0.9
true
false
none
"Lanner"
```

Numeric context is preserved through unary `+` and `-`.

## **7. Arithmetic**

```text
+  -  *  /  %
```

## **8. Bitwise**

```text
&  |  ^  ~  <<  >>
```

## **9. Logical**

```text
&&  ||  !
```

## **10. Comparisons**

```text
==  !=  <  >  <=  >=
```

## **11. Casts**

```lanner
x as i64
x as f64
```

## **12. Conditionals**

```lanner
if x > 0:
    ...
else if x == 0:
    ...
else:
    ...
```

## **13. Loops**

```lanner
while condition:
    ...
```

```lanner
for item in xs:
    ...
```

`break` and `continue` are supported.

## **14. Structs**

```lanner
Point[x: i32, y: i32]

main() i32:
    p = Point[x: 20, y: 22]
    return p.x + p.y
```

Struct fields participate in ownership and borrow checking.

## **15. Enums**

```lanner
enum ErrorCode:
    Empty
    Invalid = 7
```

## **16. Optionals**

```lanner
x: i32? = none
```

Optional payloads obey ordinary ownership rules.

## **17. Results**

```lanner
Result[i32, ErrorCode]
```

Constructors:

```text
Ok(value)
Err(error)
```

Guards:

```lanner
r is Ok(v) -> return v
r is Err(e) -> return 1
```

## **18. Fixed arrays**

```lanner
comptime N = 4
xs: [N]i32 = [1, 2, 3, 4]
```

Supported operations include construction, indexing, mutation, parameter passing, returns, struct fields, references, and bounds checking.

## **19. Dynamic arrays**

```lanner
xs: []u64 = []u64.new()
xs.push(4)
print(xs.len())
```

A typed empty array can be created directly from its element type:

```lanner
xs: []u64 = []u64.new()
```

For allocation-sensitive code, reserve the desired capacity up front:

```lanner
xs: []u64 = []u64.with_capacity(1024)
xs.reserve(4096)
print(xs.capacity())
```

`with_capacity(n)` allocates storage for at least `n` elements with length zero. `reserve(n)` raises capacity to at least `n` when needed and never shrinks an existing array. `capacity()` reports the current storage capacity.

Supported operations include:

- **ownership transfer**
- **indexing**
- **indexed mutation**
- **length and emptiness queries**
- **capacity queries**
- **explicit preallocation through `with_capacity()`**
- **explicit growth control through `reserve()`**
- **growth through `push()`**
- **slicing**
- **shared views**
- **exclusive editing views**
- **passing and returning**
- **deterministic destruction**

## **20. References**

### `&T`

A **shared, read-only, non-owning** reference.

### `&mut T`

An **exclusive, mutable, non-owning** reference.

```lanner
inc(v: &mut i64) void:
    v = v + 1
```

## **21. Views**

```lanner
view: View[i32] = xs[0:3]
```

A `View[T]` is **read-only** and **non-owning**.

An `EditView[T]` is **mutable**, **exclusive**, and **non-owning**.

## **22. Arenas**

```lanner
arena = Arena.create(256)
```

Arena-backed values share the lifetime of their region and remain subject to **escape checking**.

## **23. Comptime**

Lanner currently evaluates **integer, floating-point, boolean, and string** compile-time expressions.

```lanner
comptime N = 2 + 2
xs: [N]i32 = [10, 20, 30, 40]
```

The evaluator includes compile-time checks such as **overflow** and **division by zero** detection.

## **24. Runtime built-ins**

The current runtime/compiler-known surface includes file/environment operations, standard output operations, printing, string length, and arena creation.

Examples:

```lanner
print("hello")
print(42)
print(true)
print(3.5)
```

## **25. Bounds checking**

Indexing is checked by default. The compiler may eliminate a check only after proving the access safe.

## **26. Ownership behavior**

Non-copy values move rather than silently duplicate ownership.

A successful ownership transfer invalidates the previous owner for ownership-sensitive operations.

## **27. Memory safety**

Lanner prevents or checks for:

- **use-after-move**
- **double destruction**
- **dangling references/views**
- **conflicting exclusive access**
- **mutation through shared access**
- **unsafe dynamic-array relocation while a borrow is live**
- **invalid arena escapes**
- **out-of-bounds indexing**

See **`Memory_Model.md`** for the full semantics.

## **28. LLVM**

The production backend emits **opaque-pointer LLVM IR** and uses a selected Clang/LLVM installation.

```sh
LANNER_CLANG=/path/to/clang lanner program.lan
```

or:

```sh
LLVM_CC=/path/to/clang lanner program.lan
```

LLVM/Clang **15+** is the supported modern IR family.

## **29. CLI**

```sh
lanner program.lan
lanner program.lan -o app
lanner program.lan --run
lanner program.lan --check
lanner program.lan --emit-llvm
```


## 30. Multi-file imports

Top-level modules can be composed with:

```lanner
import "board.lan"
import "search.lan"
```

Paths are resolved relative to the importing source file. Imports are recursively expanded once; cycles and missing files are compiler errors.

## 31. NNUE-oriented SIMD

The native compiler supports signed `i8`/`i16`/`i32` vectors at 128-, 256-, and 512-bit widths. Use the `Cpu.load*`/`store*` families, explicit unaligned forms, i8-to-i16 widening, pairwise i16 multiply-adds, and i32 reductions for quantized integer kernels. New typed SIMD memory operations are explicitly unsafe.


## 32. Native integer SIMD

Lanner exposes fixed-width signed integer vectors for hand-written high-performance kernels:

```text
i8x16   i16x8   i32x4
i8x32   i16x16  i32x8
i8x64   i16x32  i32x16
```

The 128/256/512-bit widths map directly to LLVM integer vectors. The CPU surface provides aligned and explicitly unaligned loads/stores, zero construction, lane extraction, sign-extension of i8 halves, and pairwise i16 multiply-add operations. Reduction helpers such as `Cpu.reduceAddI32x8()` and `Cpu.reduceAddI32x16()` reduce an integer vector to an `i32`.

This surface is intended for NNUE accumulators and other dense integer kernels. The compiler does not silently assume a CPU ISA; select `--cpu`/`--features` for deployment-specific code paths.

## 33. Exclusive-reference optimization

`&mut T` is an exclusive access capability. The LLVM backend preserves that information by emitting `noalias` on exclusive reference parameters. This gives LLVM an explicit aliasing fact without changing Lanner's source-level ownership rules.
