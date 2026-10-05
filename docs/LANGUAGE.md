# **Stable 1.0.0 Language Reference**

This document describes the currently implemented Stable language surface.

## **1. Syntax**

Stable uses **indentation-based blocks**.

```stable
main() i32:
    print("Hello, STABLE!")
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

```stable
add(a: i32, b: i32) i32:
    return a + b
```

Function declarations are resolved independently of textual ordering in the supported compiler path, allowing **forward references and mutual recursion**.

## **5. Values**

Inference:

```stable
x = 42
```

Explicit typing:

```stable
x: i64 = 42
```

Constants:

```stable
const x = 42
```

Compile-time values:

```stable
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
"Stable"
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

```stable
x as i64
x as f64
```

## **12. Conditionals**

```stable
if x > 0:
    ...
else if x == 0:
    ...
else:
    ...
```

## **13. Loops**

```stable
while condition:
    ...
```

```stable
for item in xs:
    ...
```

`break` and `continue` are supported.

## **14. Structs**

```stable
Point[x: i32, y: i32]

main() i32:
    p = Point[x: 20, y: 22]
    return p.x + p.y
```

Struct fields participate in ownership and borrow checking.

## **15. Enums**

```stable
enum ErrorCode:
    Empty
    Invalid = 7
```

## **16. Optionals**

```stable
x: i32? = none
```

Optional payloads obey ordinary ownership rules.

## **17. Results**

```stable
Result[i32, ErrorCode]
```

Constructors:

```text
Ok(value)
Err(error)
```

Guards:

```stable
r is Ok(v) -> return v
r is Err(e) -> return 1
```

## **18. Fixed arrays**

```stable
comptime N = 4
xs: [N]i32 = [1, 2, 3, 4]
```

Supported operations include construction, indexing, mutation, parameter passing, returns, struct fields, references, and bounds checking.

## **19. Dynamic arrays**

```stable
xs = [1, 2, 3]
xs.push(4)
print(xs.len())
```

Supported operations include:

- **ownership transfer**
- **indexing**
- **indexed mutation**
- **length and emptiness queries**
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

```stable
inc(v: &mut i64) void:
    v = v + 1
```

## **21. Views**

```stable
view: View[i32] = xs[0:3]
```

A `View[T]` is **read-only** and **non-owning**.

An `EditView[T]` is **mutable**, **exclusive**, and **non-owning**.

## **22. Arenas**

```stable
arena = Arena.create(256)
```

Arena-backed values share the lifetime of their region and remain subject to **escape checking**.

## **23. Comptime**

Stable currently evaluates **integer, floating-point, boolean, and string** compile-time expressions.

```stable
comptime N = 2 + 2
xs: [N]i32 = [10, 20, 30, 40]
```

The evaluator includes compile-time checks such as **overflow** and **division by zero** detection.

## **24. Runtime built-ins**

The current runtime/compiler-known surface includes file/environment operations, standard output operations, printing, string length, and arena creation.

Examples:

```stable
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

Stable prevents or checks for:

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
STABLE_CLANG=/path/to/clang stablec program.st
```

or:

```sh
LLVM_CC=/path/to/clang stablec program.st
```

LLVM/Clang **15+** is the supported modern IR family.

## **29. CLI**

```sh
stablec program.st
stablec program.st -o app
stablec program.st --run
stablec program.st --check
stablec program.st --emit-llvm
```
