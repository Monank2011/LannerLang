# **Stable 1.0.0 Benchmarks**

Stable treats benchmarking as both a **performance measurement** and a **bug-finding instrument**.

The objective is not to declare a universal winner. The objective is to determine whether generated native code is competitive and whether severe workloads expose correctness or memory-management defects.

## **Methodology**

Where practical, the C++ comparison uses:

- **the same hardware**
- **the same LLVM/Clang toolchain**
- **the same optimization level**
- **equivalent workloads**
- **identical result validation**
- **repeated timing**
- **sanitizer instrumentation for memory tests**

Benchmark results are not counted when the workload is not mathematically equivalent or when an optimizer has removed the intended work from both programs.

---

## **Scalar compute campaign**

The scalar suite covered integer mixing, bitwise operations, branches, integer division/modulo, floating-point arithmetic, floating-point branching, function calls, conversions, signed arithmetic, and recursion.

The first campaign exposed real compiler defects in:

- **`f32` LLVM constant emission**
- **typed negative `f32` literals**
- **typed negative `i64` literals**

Those defects were repaired and converted into regression tests.

---

## **Basic memory campaign**

The basic-memory suite exercised:

- **scalar references**
- **exclusive mutable references**
- **fixed arrays**
- **dynamic arrays**
- **struct mutation**
- **nested aggregates**
- **ownership transfer**
- **`View[T]`**
- **`EditView[T]`**

It exposed two compiler issues:

- scalar **`&mut T` assignment** initially targeted the reference binding instead of its pointee;
- **`string.len()`** was missing from one backend surface.

Both were repaired and regression-tested.

---

## **Heavy memory campaign**

The heavy-memory suite exercised:

- **large dynamic allocations**
- **large reallocation/growth**
- **allocator churn**
- **nested ownership**
- **arena growth**
- **multiple large working sets**
- **large views**
- **EditView mutation**
- **repeated owner movement**
- **repeated large lifecycles**

The largest completed red-zone workload reached approximately **1.37 GiB peak RSS**.

---

## **Sanitizers**

Heavy workloads were exercised with:

- **AddressSanitizer**
- **UndefinedBehaviorSanitizer**
- **LeakSanitizer**

The completed Stable and C++ stress workloads produced **no sanitizer findings** in that campaign.

That is evidence from the executed workloads, not a mathematical proof that no future compiler bug can exist.

---

## **Performance interpretation**

The benchmark history shows Stable and optimized C++ in the same general native-performance class on the tested scalar and memory workloads.

Stable also produced several cases where its ownership/view lowering and allocation patterns were faster than the equivalent C++ workload.

Other workloads favored C++.

The project therefore avoids claiming that Stable is universally faster than C++.

The desired release property is:

```text
correctness
+
memory safety
+
native performance
+
reproducibility
```
