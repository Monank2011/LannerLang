Stable Memory Model

This document describes the current Stable 1.0.0 memory model.

The purpose of the model is to provide deterministic native memory management with strong static ownership and alias checking while avoiding mandatory garbage collection, mandatory reference counting, and explicit lifetime parameters in ordinary source code.

---

1. Core model

Stable has five primary access/ownership forms:

T
&T
&mut T
View[T]
EditView[T]

and one region ownership mechanism:

Arena

Their intended meaning is:

T            owns the value/storage
&T           shared read-only access to one value
&mut T       exclusive mutable access to one value
View[T]      shared read-only access to a range
EditView[T]  exclusive mutable access to a range
Arena        owns a region containing allocations

All references and views are non-owning.

---

2. Ownership

An owning value has responsibility for its storage.

For a non-copy value, there is one responsible owner at a time.

Ownership can move through:

- initialization
- assignment
- function arguments
- return values
- insertion into owning collections
- aggregate construction where ownership is transferred

Example:

main() i32:
    xs = [1, 2, 3]
    ys = xs

    return ys[0]

After the move, "ys" owns the dynamic array storage.

The old "xs" binding is no longer a usable owner.

Using a moved-from owner is rejected.

---

3. Copy versus move

Stable distinguishes copyable values from owning/non-copy values.

Copyable values may be duplicated according to their type semantics.

Owning values are transferred instead of silently duplicated.

This distinction survives semantic analysis and lowering.

A backend must not turn an ownership transfer into an accidental copy merely because the representation is convenient.

---

4. Use-after-move prevention

Once a non-copy value has moved:

source owner
     |
     | move
     v
destination owner

the source cannot continue to behave as though it still owns the resource.

This prevents:

- use-after-move
- double destruction
- two simultaneous owners of the same resource

---

5. Projection rules

Stable does not permit ordinary field or index access to silently steal ownership.

For example:

box.data

does not automatically move "data" merely because it appears in an expression.

Likewise:

xs[0]

does not silently extract an owning element from the containing collection.

Ownership extraction must be semantically explicit and valid.

This greatly reduces partial-move and aliasing hazards.

---

6. Shared references

"&T" is a non-owning shared read-only reference.

Properties:

- does not own the target
- may coexist with other compatible shared references
- cannot mutate the target
- may not outlive the target
- participates in lifetime/escape checking

Conceptually:

owner
  |
  +---- &T ----> read access
  |
  +---- &T ----> read access

Multiple shared readers are allowed when no conflicting exclusive access exists.

---

7. Exclusive references

"&mut T" is a non-owning exclusive mutable reference.

Properties:

- does not own the target
- grants mutation
- requires exclusive access
- conflicts with shared references
- conflicts with other exclusive references
- cannot outlive its target

Conceptually:

owner
  |
  +---- &mut T ----> exclusive access

While the exclusive borrow is active, conflicting accesses to the owner are rejected.

Example:

inc(v: &mut i64) void:
    v = 1

The reference itself remains a reference. Assignment through the binding targets the referenced object.

---

8. Views

"View[T]" is a shared range view.

Its conceptual representation is:

{
    ptr,
    len
}

The view does not own the range.

Example:

view: View[i32] = xs[0:4]

A view permits:

- reading
- indexing
- length queries
- reslicing

A view cannot mutate its backing storage.

---

9. EditView

"EditView[T]" is an exclusive mutable range view.

Conceptually:

{
    ptr,
    len
}

but with exclusive access semantics.

An active "EditView[T]" prevents conflicting:

- owner mutation
- owner access
- shared view access
- conflicting mutable view/reference access

The compiler keeps the exclusive borrow active until it is no longer needed.

---

10. Owner and view interaction

The owner remains the owner after creating a view.

Creating a view does not move ownership.

For example:

xs = [1, 2, 3, 4]
view: View[i32] = xs[1:3]

"xs" still owns the storage.

However, operations that could invalidate the view are blocked while the view remains active.

This is particularly important for dynamic arrays.

---

11. Dynamic arrays

A dynamic array is conceptually:

{
    data,
    len,
    cap,
    arena
}

where:

data   pointer to element storage
len    number of initialized elements
cap    available capacity
arena  ownership/allocation context

The exact ABI may evolve, but the ownership contract is the important semantic property.

Dynamic arrays support:

- indexing
- indexed mutation
- length
- emptiness checks
- push
- slicing
- ownership transfer
- deterministic destruction

---

12. Reallocation

"push()" may grow an array.

If growth requires relocation:

old storage
     |
     | reallocate
     v
new storage

existing references/views into the old storage would become invalid.

Therefore Stable rejects potentially relocating mutations while conflicting borrows/views remain active.

This rule is semantic.

The compiler may optimize a mutation when it can prove relocation cannot happen, but correctness must never depend on an optimization accident.

---

13. Borrow expiry

Stable does not require programmers to write lifetime parameters.

Instead, the compiler infers when an access is no longer needed.

The implementation uses last-use analysis so a borrow can end after its final useful use.

Conceptually:

xs = [1, 2, 3]
view = xs[:]

x = view[0]

# after the final use of view,
# the borrow may end

xs.push(4)

The important property is:

borrow begins
    ->
borrow remains active while needed
    ->
borrow ends at last valid use

---

14. Storage origins

Stable tracks where storage comes from.

Important origin categories include:

local
parameter
heap-owned
arena
global/static

A reference/view carries provenance information describing its source storage.

A short-lived reference cannot be stored, returned, or captured in a way that would allow it to outlive its source.

---

15. Escape analysis

Examples of invalid escapes include:

returning a reference to a dead local
storing a local view inside a longer-lived owner
keeping a view after its array has been destroyed
keeping an arena reference after its arena is gone

The compiler compares the source and destination lifetimes/regions.

The general rule is:

destination lifetime
must not exceed
source lifetime

unless the compiler can establish a valid longer-lived origin.

---

16. Arenas

"Arena" provides region-based ownership.

A conceptual arena:

Arena
  |
  +-- allocation A
  +-- allocation B
  +-- allocation C
  +-- ...

Allocations belonging to the region share its lifetime.

The intended runtime strategy is chunked bump allocation.

Desired properties:

- fast allocation
- predictable alignment
- stable addresses for existing allocations
- bulk release
- low per-object overhead

---

17. Arena address stability

Growing an arena must not relocate previously allocated objects merely because a new chunk is required.

This allows stable references within an arena region.

However, a reference still cannot outlive the arena itself.

---

18. Arena escape rules

An arena-backed object cannot safely escape the lifetime of its region.

For example, a function must not return an arena-owned array if doing so would leave the returned value dependent on a region that has already ended.

The same principle applies to:

- "&T"
- "&mut T"
- "View[T]"
- "EditView[T]"

---

19. Stack, heap, and region storage

Stable does not require all values to use one storage class.

Values may be represented using:

stack storage
heap storage
arena/region storage

The physical storage strategy is separate from semantic ownership.

The compiler is free to choose an efficient representation when semantics permit it.

---

20. Deterministic destruction

Stable uses ownership-driven cleanup instead of tracing collection.

Owning values are destroyed exactly once when ownership ends.

Nested aggregates are recursively cleaned.

Current coverage includes:

- dynamic arrays
- structs
- Optional values
- Result values
- nested aggregates
- arena-related owning structures

The backend must emit cleanup paths consistently across supported lowering paths.

---

21. Move/destruction invariant

For every owning value:

one live owner
      |
      +-- zero or more valid ownership transfers
      |
      v
one final destruction

A moved-from value must not be destroyed as though it still owned the previous storage.

---

22. Optional and Result ownership

Optional and Result values participate in ordinary ownership rules.

For:

T?

or:

Result[T, E]

the payload may itself be owning.

The compiler must account for ownership when:

- constructing the value
- transferring it
- extracting/binding it
- returning it
- destroying it

A non-copy payload is never duplicated merely because it appears inside an Optional or Result.

---

23. Result guards

Guard expressions such as:

r is Ok(v)

must respect the ownership of "r".

Non-copy payloads are handled using borrow-safe bindings rather than silently duplicating ownership.

---

24. Loop borrows

Loop iteration can create temporary borrows of elements.

These borrows participate in the same alias rules as ordinary references.

Therefore a collection cannot be mutated in a way that conflicts with a live iteration borrow.

The compiler may end an element borrow after its last use.

---

25. Mutation rules

Mutation requires exclusive access to the affected storage.

Therefore:

&T           cannot mutate
View[T]      cannot mutate
&mut T       may mutate exclusively
EditView[T]  may mutate exclusively
owner        may mutate only without conflicts

---

26. Aliasing

The compiler obtains strong alias information from the memory model.

The semantic categories are:

shared + shared
    allowed when non-conflicting

shared + exclusive
    rejected while overlapping

exclusive + exclusive
    rejected while overlapping

This information is useful not only for safety but also for optimization.

Potential optimization uses include:

- load/store forwarding
- dead-store elimination
- register promotion
- bounds-check elimination
- alias analysis
- vectorization
- LLVM optimization

---

27. Bounds safety

Indexing is checked by default.

A runtime failure traps deterministically.

The compiler may eliminate a bounds check if it can prove safety from:

- fixed-size array dimensions
- known lengths
- loop ranges
- dominating comparisons
- range/view invariants
- compile-time constants

The proof must be conservative.

An invalid proof must never remove a required safety check.

---

28. No tracing GC

The core Stable memory model does not require a tracing garbage collector.

Ordinary ownership is explicit through types and compiler semantics.

There is no hidden global collector deciding when arbitrary program objects disappear.

---

29. No mandatory reference counting

Stable does not silently transform ordinary values into reference-counted objects.

The memory model is intended to avoid pervasive:

atomic increment
atomic decrement
reference-count bookkeeping

unless a future explicitly shared-ownership abstraction requests such behavior.

---

30. Runtime representation

The safety system is intended to be primarily compile-time.

Semantic ownership information is carried through compiler IR metadata and lowering decisions.

That metadata is not intended to become a mandatory runtime ownership ledger.

Conceptual representations remain compact:

dynamic array:
    {data, len, cap, arena}

View/EditView:
    {ptr, len}

---

31. Self-hosted compiler memory model

The self-hosted compiler uses an arena-backed node representation with stable identifiers.

Conceptually:

Arena
  |
  +-- node 0
  +-- node 1
  +-- node 2
  +-- ...

A "NodeId" identifies a node in the compiler's arena-backed node table.

This allows compiler data to use stable identifiers without turning every compiler node into an owning pointer relationship.

---

32. Backend contract

Ownership decisions are made before machine-code generation.

Backends must preserve:

- move semantics
- borrow semantics
- lifetime validity
- storage provenance
- destruction
- bounds checks
- safe memory operations

The backend is not allowed to invent a different ownership model.

---

33. Non-negotiable safety invariants

The core model aims to preserve these invariants:

1. No use-after-free.
2. No use-after-move.
3. No double destruction.
4. No dangling reference.
5. No dangling view.
6. No conflicting exclusive aliases.
7. No mutation through shared access.
8. No invalidating relocation while a conflicting borrow is active.
9. No invalid lifetime/region escape.
10. One responsible owner at a time.
11. Deterministic destruction.
12. Efficient native representations.

---

34. Mental model

The entire system can be remembered as:

T
    I own it.

&T
    I may read it temporarily.

&mut T
    I may edit it temporarily and exclusively.

View[T]
    I may read this range temporarily.

EditView[T]
    I may edit this range temporarily and exclusively.

Arena
    This region owns these allocations.

Stable's compiler performs the bookkeeping.

The programmer should reason about ownership and access, rather than manually naming lifetime parameters.

---

35. What this model intentionally does not claim

The memory model is a language/compiler design and implementation, not a mathematical proof of the absence of every possible compiler defect.

Testing, sanitizer runs, fuzzing, formal verification, code review, and continued compiler validation remain necessary.

The goal is for memory safety to be enforced by construction in the language implementation rather than depending on programmers remembering undocumented conventions.
