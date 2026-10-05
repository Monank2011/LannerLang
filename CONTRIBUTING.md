# **Contributing to Stable**

Stable is both a language and a compiler. Contributions should preserve **language semantics**, **generated-code correctness**, and **memory-model invariants**.

## **Build**

```sh
cmake -S . -B build -DSTABLE_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## **Compiler changes**

When changing compiler behavior, add a regression test whenever the behavior is observable.

Especially test:

- **ownership and move rules**
- **borrow conflicts**
- **escape analysis**
- **destruction**
- **bounds checks**
- **numeric typing**
- **Optional/Result behavior**
- **LLVM lowering**
- **self-hosting**
- **LLVM compatibility**

## **Benchmark changes**

A performance test should validate the result as well as measure time. Do not claim a performance advantage from a workload that is not semantically equivalent or has been optimized away.

## **Memory-model rule**

The source-level memory model is authoritative. A backend must implement it rather than weaken it to make a program compile.
