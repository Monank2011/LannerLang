# **Lanner 3.0.0 Benchmarks**

Lanner treats benchmarking as both a **performance measurement** and a **bug-finding instrument**.

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

The completed Lanner and C++ stress workloads produced **no sanitizer findings** in that campaign.

That is evidence from the executed workloads, not a mathematical proof that no future compiler bug can exist.

---

## **Native performance controls**

The production LLVM path now includes several backend-level optimizations for hot systems code:

- dynamic-array header scalarization on `push`;
- direct scalar-field access for local dynamic-array indexing;
- conservative compile-time fixed-index range elimination;
- hot/cold branch weighting for bounds and allocation failure paths;
- allocator alias/size metadata;
- optional host CPU tuning with `--native`.

For highest host-local performance, build with:

```text
lanner file.lan -O3 --native
```

`--native` is intentionally opt-in because it trades portability for CPU-specific tuning.

## **Performance interpretation**

The benchmark history shows Lanner and optimized C++ in the same general native-performance class on the tested scalar and memory workloads.

Lanner also produced several cases where its ownership/view lowering and allocation patterns were faster than the equivalent C++ workload.

Other workloads favored C++.

The project therefore avoids claiming that Lanner is universally faster than C++.

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


## **3.0.0 measurements**

Single-machine, single-thread sandbox (1 vCPU, clang 18, `-O3`), wall-clock seconds, one run each after a warm-up run, so differences of a few percent are within noise.
The compute kernels (fib(40), a 100M sieve, 700x700 f64 matmul, 800M-iteration xorshift mix) were *already* at C parity before 3.0.0; the numbers below show 3.0.0 did not give that up and where it helps.

| Kernel | Lanner before | Lanner 3.0.0 | C (clang -O3) |
|---|---|---|---|
| fib(40) | 0.332 | 0.287 | 0.289 |
| sieve 100M | 1.406 | 1.326 | 1.353 |
| matmul 700 | 0.140 | 0.112 | 0.119 |
| xorshift mix 800M | 1.943 | 1.896 | 1.862 |
| `examples/perf_hot.lan` | ~0.08 | ~0.06 | ~0.07 |

Compile-side effect of outlining the array-growth path: the selfhost compiler (`selfhost/compiler.lan`) lowers to 80,889 LLVM IR lines instead of 88,098, and its `-O2` native build dropped from about 6.7 s to about 5.7 s.

An intermediate version passed the array header *address* to the growth helper and made matmul about 2x slower because the header escaped and stopped living in registers. The shipped helper takes data/len/cap by value; the `cli_v3_outlined_array_grow` test guards the IR shape.
