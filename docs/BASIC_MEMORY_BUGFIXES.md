# Step 2 Basic-Memory Benchmark Repairs

The basic-memory benchmark exposed two production compiler issues that are fixed in this checkpoint.

## 1. Scalar `&mut T` lvalue assignment

A plain assignment such as:

```lanner
inc(v: &mut i64) void:
    v = 1
```

was incorrectly treated as an attempt to assign a new reference value to the parameter slot. Lanner now treats a mutable-reference identifier as an lvalue for its pointee, while keeping the reference binding itself unchanged.

The fix is implemented consistently in:

- semantic assignment checking;
- the direct LLVM backend;
- the legacy HIR lowerer;
- the HIR identifier read path, so arithmetic through `&mut T` loads the pointee instead of the reference pointer bits.

## 2. `string.len()` backend parity

The type checker already accepted `string.len()`, but the production LLVM backend and legacy HIR lowering did not lower that method correctly. Both now lower it through the same `strlen` runtime primitive used by `stringLen(string)`.

## Validation

- Default CTest: **34/34 PASS**
- Legacy HIR: **55/57 executed tests PASS**; the two remaining recursive self-host tests exceeded the sandbox execution wall before completion.
- Native LLVM self-host bootstrap: **PASS**, including byte-identical stage 2/stage 3 LLVM.
- Typed self-host frontend: **PASS**
- LLVM compatibility regression: **PASS**
- Native readFile failure safety: **PASS**
- Original Step 2 basic-memory correctness suite: **16/16 Lanner/C++ matches** after the repairs.
- Dedicated scalar mutable-reference reproduction: executable now returns **1**, matching the Lanner source semantics `x = 0; inc(&mut x); return x as i32`.

No heavy-memory benchmark was run in this repair checkpoint.
