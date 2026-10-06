# Lanner Systems & Low-Level Support

Lanner 1.0.0 now exposes a complete native systems-oriented layer around the ownership-safe language core. The safety boundary is explicit: ordinary code remains borrow-checked, while raw hardware-oriented operations require `unsafe`.

## Memory and pointers

- `*T` and `*mut T` raw pointers
- `&raw` address creation
- raw pointer dereference and unchecked indexing
- pointer `add`, `sub`, and `offset` methods
- `ptrDiff()` for element-distance
- integer/raw-pointer casts
- `null` raw-pointer values
- `alloc`, `realloc`, `dealloc`
- `allocAligned`, `deallocAligned`
- `stackAlloc` with explicit power-of-two alignment
- `volatileLoad` / `volatileStore`
- `unalignedLoad` / `unalignedStore`
- `memset`, `memcpy`, `memmove`, `memcmp`
- `sizeOf`, `alignOf`, `offsetOf`

Raw pointer values are copyable and deliberately outside the safe borrow ledger. Creation, arithmetic, dereference, and pointer/integer conversion remain `unsafe`, so the optimizer can still distinguish ordinary references from escape-hatch operations.

## Layout and storage control

Lanner structs use target ABI layout by default. Packed structures use `packed Name[...]`. Static storage supports compile-time initialization, explicit `align(N)`, custom ELF/COFF/Mach-O section names, mutability, and thread-local storage.

```lanner
packed Packet[opcode:u8, flags:u8, value:u32]
static align(64) section ".fastdata" table:[8]u64=[0,1,2,3,4,5,6,7]
static threadLocal mut scratch:u64=0
```

C data symbols can be imported directly:

```lanner
extern static mut errno:i32
```

Functions and external symbols are emitted with their Lanner names, so explicit linker/ABI integration does not require a name-mangling layer. Scalar, pointer, string-pointer, and function-pointer forms map directly to the C ABI. By-value aggregate FFI is deliberately rejected unless represented through a pointer or an explicit C wrapper, avoiding silent ABI miscompiles across targets.

## Unsafe functions and function pointers

Unsafe functions are tracked in function-pointer types:

```lanner
unsafe fn touch(p:*mut i32) i32:
    return *p

main() i32:
    x:i32=41
    unsafe:
        f:unsafe fn(*mut i32)i32=touch
        return f(&raw x)
```

An unsafe function pointer cannot be called from safe code. This keeps the low-level escape explicit even after a function has been stored or passed through an API.

## Inline assembly

Inline assembly is explicit and requires `unsafe`. Lanner supports void assembly plus integer/pointer-valued forms: `asm`, `asmI32`, `asmI64`, and `asmPtr`. Templates and LLVM constraint strings are passed directly to LLVM, including clobber lists and side-effect semantics.

This is sufficient for architecture-specific instructions, syscall glue, barriers, bootstrap code, and MMIO-adjacent helpers.

## Atomic and CPU primitives

The native runtime exposes OS threads and `Atomic[T]`, including configurable load/store/fetch/CAS memory orderings. `atomicFence()` and `compilerFence()` are available for explicit ordering barriers. CPU intrinsics include feature probing, cycle counters, popcount/ctz/clz/bswap, BMI2, prefetch, and fixed-width vector values.

## Freestanding and cross compilation

The compiler can emit LLVM IR, assembly, or object files for a selected target triple. Executable builds support:

```text
--target=<triple>
--sysroot <path>
--cpu <name>
--features <f1,f2,...>
--linker <name>
--linker-script <path>
--entry <symbol>
--freestanding
--no-runtime
```

`--freestanding` removes the Lanner hosted runtime and hosted startup/default libraries, and links with the selected entry symbol. This enables bootloader, kernel, firmware, and other no-runtime targets while leaving allocator/device/platform services to explicit FFI or inline assembly.

For example, an x86-64 freestanding entry can be built without a `main` function:

```lanner
unsafe _start() i32:
    syscall:i64=60
    status:i64=42
    asm("mov $0, %rax; mov $1, %rdi; syscall", "r,r,~{rax},~{rdi},~{rcx},~{r11}", syscall, status)
    return 0
```

The emitted object can also be cross-compiled without linking a host executable, for example to `i386-unknown-linux-gnu` or `aarch64-none-elf`. The final linker, sysroot, and target SDK remain external toolchain inputs, as they do for C, C++, and Rust.

## Systems completeness boundary

The language/compiler layer now contains the primitives needed for C/Rust-class systems programming: no-GC native code, raw pointers, manual and stack allocation, target-sized integers, explicit ABI/FFI, volatile/un-aligned memory access, atomics, SIMD/CPU intrinsics, inline assembly, static storage control, and freestanding/cross-target output.

Platform SDKs and device-specific APIs are intentionally not baked into the compiler. They belong in the standard library, target support crates/packages, or explicit FFI bindings.
