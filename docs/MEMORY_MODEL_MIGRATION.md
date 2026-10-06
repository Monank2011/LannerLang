# Memory Model Migration

This bootstrap tree now uses the canonical memory model in `docs/MEMORY_MODEL.md`.

## Semantic core

- `Symbol` ownership/borrow state is consolidated in `lanner::memory::BindingState`.
- Lanner binding identities allow borrow tokens to follow moves and shadowed bindings safely.
- `TypeNode::origin` is the single provenance field for local, parameter, arena, heap, and global storage origins.
- Borrow modes are shared or exclusive and apply uniformly to `&T`, `&mut T`, `View[T]`, and `EditView[T]`.
- Borrow expiry is compiler-inferred using last-use analysis. An unused borrow ends after its defining statement; a borrow used later remains active until its final use.
- Dynamic-array mutation is rejected while any active borrow/view could be invalidated by relocation.
- Owning transfers retarget ordinary storage provenance to the destination owner, while arena-backed ownership retains its region origin.

## HIR/backend contract

HIR `Type` now carries `MemoryKind` and `StorageOrigin`, so the front end's ownership/borrow decisions survive lowering. ABI representations remain native: dynamic arrays remain `{data,len,cap,arena}` and views remain `{ptr,len}`. The metadata is semantic and does not add runtime bookkeeping.

## Removed legacy state

The previous `Symbol` fields (`isMoved`, separate borrow counters/flags, `borrowedFrom`, and `arenaTag`) and the `TypeNode` provenance mirrors (`arenaTag`, `viewFromLocal`, `borrowOrigin`) were removed. Backends now copy the canonical `origin` metadata instead of carrying parallel legacy fields.

## Validation

The native/default build passes 29 configured tests. The legacy-HIR build passes the 33-test bootstrap/self-host block and the 15 remaining compatibility/memory tests separately. The dedicated bootstrap-stage comparison test did not finish within the available test window; no compiler failure was reported before the timeout.
