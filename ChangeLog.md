# **Lanner Changelog**

## **3.0.0 — Performance and ergonomics release**

### Language
- Compound assignment operators: `+= -= *= /= %= &= |= ^= <<= >>=` on variables, fields and indexed elements.
- Range loops: `for i in a..b` and `for i in a..=b` with literal-bound type adoption, read-only loop variable, and overflow-safe inclusive ranges.
- `elif` as shorthand for `else if`.
- Digit separators in integer and float literals (`1_000_000`, `0xFF_FF`, `0b1010_0101`).

### Performance
- Dynamic-array growth outlined into one shared by-value `cold noinline` helper; the selfhost compiler's generated IR shrank from 88,098 to about 80,900 lines and its `-O2` build got faster.
- Internal linkage for whole-program hosted executables (`--keep-symbols` opts out; skipped for objects, web, freestanding and `--link` builds).
- Counted loops emit `add nuw`/`nsw` increments for exact trip counts.

### Tooling and tests
- New `--keep-symbols` flag.
- 10 new CTest cases (language features at `-O0`/`-O2`, must-fail diagnostics, outlined-grow IR shape).
- Version bumped to 3.0.0 in `VERSION`, CMake, `lanner --version`, installers, Python packaging, the VS Code extension and the documentation. Earlier changelog entries below are historical and keep their original version numbers.

## **1.0.0**

### Ergonomics additions

- **Typed empty dynamic arrays via `[]T.new()`**
- **`[]T.with_capacity(n)`, `reserve(n)`, and `capacity()`**

Lanner 1.0.0 marks the first release checkpoint of the current implemented Lanner language/compiler surface.

### **Language**

- **Indentation-based syntax**
- **Static typing**
- **Integers and floating point (`f32`, `f64`)**
- **Booleans and strings**
- **Fixed and dynamic arrays**
- **Structs and enums**
- **Optional values**
- **Result values**
- **References**
- **Views and EditViews**
- **Arenas/regions**
- **`if`, `else if`, `while`, and `for`**
- **`break` and `continue`**
- **Comptime expressions**
- **Forward references and mutual recursion**
- **Arithmetic, logical, comparison, and bitwise operators**

### **Memory model**

- **Move/use-after-move checking**
- **Shared `&T` borrows**
- **Exclusive `&mut T` borrows**
- **Borrow lifetime inference**
- **Storage-origin tracking**
- **Escape checking**
- **Dynamic-array invalidation safety**
- **Arena lifetime checking**
- **Deterministic destruction**
- **Nested ownership cleanup**
- **Bounds checking and safe elimination**

### **Compiler**

- **Native LLVM production backend**
- **Optional legacy HIR backend**
- **LLVM/Clang toolchain selection**
- **LLVM compatibility tooling**
- **Native self-hosting**
- **Recursive bootstrap verification**
- **Source-aware diagnostics**

### **Benchmark-discovered repairs**

The development benchmark campaign exposed and fixed several subtle compiler defects, including:

- **LLVM floating-point constant emission**
- **typed negative `f32` literals**
- **typed negative `i64` literals**
- **scalar `&mut T` assignment**
- **pointee loading through mutable references**
- **`string.len()` backend parity**

---

## **Development history**

Earlier Lanner development milestones include the prototype compiler, memory-model redesign, native LLVM migration, self-hosting work, HIR/SSA infrastructure, and bootstrap milestones.

The detailed historical commits remain available in the Git history.
