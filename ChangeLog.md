Changelog

All notable Stable changes are documented here.
1.0.0
Language
Expanded native LLVM feature parity across scalars, aggregates, arrays, references, views, Optional, Result, control flow, and comptime.
Added or completed support for fixed arrays.
Added dynamic-array .len(), .isEmpty(), and .push().
Added View[T] and EditView[T].
Added native Optional and Result lowering.
Added Ok(...) and Err(...).
Added is Ok(...) and is Err(...) guards.
Added else if.
Added for, break, and continue.
Added hexadecimal and binary integer literals.
Added f32 and f64 arithmetic paths.
Added contextual numeric handling for unary + and -.
Added local and global comptime values.
Added compile-time fixed-array dimensions.
Added print(...).
Memory model
Consolidated ownership and borrow tracking into the canonical memory model.
Added shared &T references.
Added exclusive &mut T references.
Added shared View[T].
Added exclusive EditView[T].
Added storage-origin tracking.
Added inferred borrow expiry using last-use analysis.
Added ownership transfer checks.
Added move/use-after-move protection.
Added nested aggregate destruction.
Added dynamic-array relocation safety while conflicting borrows are active.
Added arena-backed ownership and escape checking.
Added recursive ownership handling for Optional and Result payloads.
Rejected implicit owning moves from field/index projections.
Added borrow-aware loop iteration.
Compiler/backend
Native LLVM is the production backend.
Legacy HIR remains available as an optional compatibility/bootstrap backend.
Added LLVM/Clang toolchain selection through STABLE_CLANG and LLVM_CC.
Added LLVM 15+ compatibility enforcement for modern opaque-pointer IR.
Added LLVM portability verification tooling.
Added native self-hosting verification.
Added recursive bootstrap checks.
Added deterministic runtime bounds/allocation traps.
Added source-aware diagnostics.
Bug fixes discovered by benchmarks
Fixed LLVM floating-point constant emission.
Fixed typed negative f32 literals.
Fixed typed negative i64 literals.
Fixed scalar &mut T assignment.
Fixed pointee loading through mutable references.
Fixed string.len() lowering across supported backend paths.
0.5.x
Stable self-hosting, typed HIR, native LLVM replacement, ownership-aware cleanup, Optional/Result, fixed arrays, views, and bootstrap infrastructure were developed through the 0.5 development series.
See repository history for individual bootstrap milestones. 
