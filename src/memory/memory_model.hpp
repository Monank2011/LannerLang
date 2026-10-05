#pragma once

#include <cstdint>
#include <stdexcept>
#include <optional>
#include <string>

namespace stable::memory {

enum class OwnershipState : std::uint8_t {
    Live,
    Moved,
};

enum class BorrowMode : std::uint8_t {
    Shared,
    Exclusive,
};

enum class StorageOriginKind : std::uint8_t {
    Unknown,
    Local,
    Parameter,
    Arena,
    Heap,
    Global,
};

struct StorageOrigin {
    StorageOriginKind kind = StorageOriginKind::Unknown;
    std::string binding;

    bool valid() const { return kind != StorageOriginKind::Unknown; }
    bool isShortLived() const {
        return kind == StorageOriginKind::Local || kind == StorageOriginKind::Arena;
    }
};

struct BorrowRecord {
    BorrowMode mode = BorrowMode::Shared;
    StorageOrigin origin;
    std::string ownerBinding;
    std::uint64_t ownerId = 0;
};

// Single authoritative semantic state for one binding. Ownership and access
// permissions are deliberately kept together so moves, borrows, and release
// cannot drift apart across separate boolean fields.
struct BindingState {
    OwnershipState ownership = OwnershipState::Live;
    std::uint32_t sharedBorrows = 0;
    bool exclusiveBorrow = false;
    std::optional<BorrowRecord> borrow;

    bool isMoved() const { return ownership == OwnershipState::Moved; }
    bool hasAnyBorrow() const {
        return sharedBorrows != 0 || exclusiveBorrow;
    }
    bool hasExclusiveBorrow() const { return exclusiveBorrow; }

    void markMoved() { ownership = OwnershipState::Moved; }
    void markLive() { ownership = OwnershipState::Live; }
};

inline bool mayBeginBorrow(const BindingState& owner, BorrowMode mode) {
    if (owner.isMoved()) return false;
    if (mode == BorrowMode::Exclusive) {
        return !owner.exclusiveBorrow && owner.sharedBorrows == 0;
    }
    return !owner.exclusiveBorrow;
}

inline bool mayMove(const BindingState& owner) {
    return !owner.isMoved() && !owner.hasAnyBorrow();
}

inline void beginBorrow(BindingState& owner, BorrowRecord borrow) {
    if (!mayBeginBorrow(owner, borrow.mode)) {
        throw std::logic_error("invalid overlapping borrow");
    }
    if (borrow.mode == BorrowMode::Exclusive) {
        owner.exclusiveBorrow = true;
    } else {
        ++owner.sharedBorrows;
    }
}

inline void endBorrow(BindingState& owner, const BorrowRecord& borrow) {
    if (borrow.mode == BorrowMode::Exclusive) {
        if (owner.exclusiveBorrow) owner.exclusiveBorrow = false;
    } else if (owner.sharedBorrows != 0) {
        --owner.sharedBorrows;
    }
}

inline const char* originName(StorageOriginKind kind) {
    switch (kind) {
        case StorageOriginKind::Unknown: return "unknown";
        case StorageOriginKind::Local: return "local";
        case StorageOriginKind::Parameter: return "parameter";
        case StorageOriginKind::Arena: return "arena";
        case StorageOriginKind::Heap: return "heap";
        case StorageOriginKind::Global: return "global";
    }
    return "unknown";
}

} // namespace stable::memory
