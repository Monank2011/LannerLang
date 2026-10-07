# **Lanner 2.0.0 Memory Model**

Lanner's memory model is built around **ownership**, **exclusive access**, **non-owning views**, **storage origins**, and **regions**.

The central rule is simple:

> **Owning storage has one responsible owner, while temporary access is explicitly borrowed and never silently becomes ownership.**

---

## **1. Access forms**

```text
T            owned value
&T           shared read-only borrow
&mut T       exclusive mutable borrow
View[T]      shared read-only range
EditView[T]  exclusive mutable range
Arena        region owner
```

References and views are **non-owning**.

---

## **2. Ownership**

An owning value has one responsible owner at a time.

For non-copy values, ownership can move through:

- initialization
- assignment
- function arguments
- returns
- collection insertion
- aggregate construction

After a move, the old binding cannot continue to act as the owner.

This gives the core invariant:

```text
one responsible owner
        |
        | ownership transfers
        v
one responsible owner
        |
        v
one final destruction
```

---

## **3. Copy versus move**

**Copyable values** may be duplicated according to their type semantics.

**Owning/non-copy values** are transferred rather than silently duplicated.

The backend must preserve that distinction.

---

## **4. Use-after-move**

Once ownership moves:

```text
source --move--> destination
```

the source is no longer a valid owner.

This prevents accidental reuse and later double destruction.

---

## **5. Projection safety**

Field and index expressions do not silently extract owning subobjects merely because they appear in an expression.

This avoids accidental partial moves from structures and collections.

---

## **6. Shared borrowing**

`&T` provides **read-only, non-owning** access.

Multiple compatible shared borrows may coexist.

A shared borrow conflicts with an exclusive mutable access over overlapping storage.

---

## **7. Exclusive borrowing**

`&mut T` provides **exclusive, mutable, non-owning** access.

Example:

```lanner
inc(v: &mut i64) void:
    v = v + 1
```

While the exclusive borrow is live, conflicting owner access and conflicting borrows are rejected.

The borrow itself does not acquire ownership.

---

## **8. Borrow lifetime inference**

Lanner does not require explicit lifetime parameter syntax.

The compiler determines when a borrow is needed and can end it after its last relevant use.

Conceptually:

```text
borrow begins
    |
    v
uses of borrow
    |
    v
last use
    |
    v
borrow ends
```

This is particularly important for permitting mutation again as soon as a temporary access is finished.

---

## **9. Views**

`View[T]` is a **read-only, non-owning range**.

`EditView[T]` is an **exclusive mutable, non-owning range**.

Conceptually both contain a range descriptor such as:

```text
pointer/reference information + length
```

Neither view becomes the owner of its backing storage.

---

## **10. Dynamic arrays and invalidation**

A dynamic array logically contains:

```text
data
length
capacity
allocation/region ownership information
```

A growth operation may need to relocate the backing storage.

Therefore Lanner prevents potentially relocating mutation while a conflicting reference or view remains live.

A valid optimization may avoid the actual relocation when capacity is sufficient, but **safety is determined semantically**, not by hoping an optimization happens to preserve an address.

---

## **11. Bounds safety**

Array and view indexing is checked unless the compiler proves the access safe.

Proof information can come from:

- **fixed array size**
- **known lengths**
- **loop ranges**
- **dominating conditions**
- **view lengths**
- **compile-time values**

Invalid runtime indexing traps deterministically.

---

## **12. Storage origins**

Lanner tracks the provenance of storage for lifetime/escape checking.

Relevant origins include storage associated with:

- local variables
- function parameters
- owned dynamic storage
- arena regions
- longer-lived/global storage

A temporary reference cannot be returned, stored, or otherwise escaped in a way that outlives the storage it refers to.

---

## **13. Escape analysis**

The fundamental rule is:

```text
destination lifetime
must not exceed
source lifetime
```

unless the compiler has established a valid longer-lived origin.

This applies to:

- `&T`
- `&mut T`
- `View[T]`
- `EditView[T]`
- arena-backed values

---

## **14. Arenas**

`Arena` provides **region-based ownership**.

Conceptually:

```text
Arena
 |
 +-- allocation A
 +-- allocation B
 +-- allocation C
 +-- ...
```

Arenas are useful when many objects share one lifetime boundary.

The implementation uses a native region allocator and maintains lanner addresses for existing allocations when the region grows by obtaining additional storage.

---

## **15. Arena escape safety**

A reference or view tied to an arena cannot escape the region in a way that would allow use after region destruction.

The same rule applies recursively to aggregate values containing arena-backed storage.

---

## **16. Destruction**

Lanner uses **deterministic, ownership-driven destruction** rather than a tracing collector for ordinary ownership.

Nested cleanup is tracked for supported owning structures including:

- dynamic arrays
- structs
- Optional payloads
- Result payloads
- nested aggregates

A moved-from owner is not destroyed as though it still owned the original resource.

---

## **17. Optional and Result ownership**

An Optional or Result can contain an owning payload.

Construction, transfer, extraction, return, and destruction must all preserve the payload's ownership state.

The compiler cannot silently duplicate an owning payload merely because it is wrapped in another type.

---

## **18. Loop borrows**

`for` iteration may create temporary borrows of elements.

Those borrows obey the same conflict rules as ordinary references.

The compiler can end the loop-held access after its final use.

---

## **19. Alias rules**

The fundamental relationships are:

```text
shared + shared
    allowed when compatible

shared + exclusive
    rejected while overlapping

exclusive + exclusive
    rejected while overlapping
```

This information is useful for both **safety** and **optimization**.

---

## **20. No tracing GC requirement**

Ordinary Lanner ownership does not require tracing garbage collection.

Objects are released according to ownership and region lifetimes.

---

## **21. No hidden reference counting**

Lanner does not silently make every ordinary object reference-counted.

This avoids imposing reference-count bookkeeping and synchronization costs on code that does not request shared ownership semantics.

---

## **22. Backend contract**

LLVM lowering must preserve:

- **ownership transfers**
- **borrow exclusivity**
- **storage provenance**
- **destruction**
- **bounds checks**
- **region lifetime**

The backend is an implementation of the memory model, not an alternative memory model.

---

## **23. Safety invariants**

The implementation aims to preserve these invariants:

1. **No use-after-move.**
2. **No double destruction.**
3. **No dangling references/views.**
4. **No conflicting exclusive aliases.**
5. **No mutation through shared access.**
6. **No invalidating relocation while a conflicting borrow is live.**
7. **No invalid region escape.**
8. **Bounds-safe indexing.**
9. **One responsible owner at a time.**
10. **Deterministic destruction.**

---

## **24. Important qualification**

A memory-safe language design does not make a compiler mathematically infallible.

Lanner therefore relies on multiple layers of validation:

- semantic regression tests
- backend tests
- bootstrap tests
- sanitizer tests
- adversarial memory workloads
- generated-code validation
- ongoing fuzzing and review

The model is designed so that common memory errors are rejected by construction rather than left entirely to programmer discipline.
