# **Stable Changelog**

## **1.0.0**

Stable 1.0.0 marks the first release checkpoint of the current implemented Stable language/compiler surface.

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

Earlier Stable development milestones include the prototype compiler, memory-model redesign, native LLVM migration, self-hosting work, HIR/SSA infrastructure, and bootstrap milestones.

The detailed historical commits remain available in the Git history.
