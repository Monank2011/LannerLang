#pragma once

#include <cstdint>
#include <stdexcept>
#include <optional>
#include <string>
#include <vector>
#include <algorithm>

namespace lanner::memory {

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
    std::string place;
    std::uint64_t ownerId = 0;
};

// Single authoritative semantic state for one binding. Ownership and access
// permissions are deliberately kept together so moves, borrows, and release
// cannot drift apart across separate boolean fields.
struct BindingState {
    OwnershipState ownership = OwnershipState::Live;
    // Borrow tokens held by this binding when it is itself a reference/view.
    std::optional<BorrowRecord> borrow;
    // Active borrows granted from this binding's storage. Multiple entries are
    // allowed when their places are disjoint or both are shared borrows.
    std::vector<BorrowRecord> activeBorrows;

    bool isMoved() const { return ownership == OwnershipState::Moved; }
    bool hasAnyBorrow() const { return !activeBorrows.empty(); }
    bool hasExclusiveBorrow() const {
        return std::any_of(activeBorrows.begin(), activeBorrows.end(), [](const BorrowRecord& r) {
            return r.mode == BorrowMode::Exclusive;
        });
    }
    void markMoved() { ownership = OwnershipState::Moved; }
    void markLive() { ownership = OwnershipState::Live; }
};

inline bool placesOverlap(const std::string& a, const std::string& b) {
    if (a.empty() || b.empty()) return true;
    if (a == b) return true;
    const auto prefix = [](const std::string& parent, const std::string& child) {
        return child.size() > parent.size() && child.compare(0, parent.size(), parent) == 0 && child[parent.size()] == '.';
    };
    return prefix(a, b) || prefix(b, a);
}

inline bool mayBeginBorrow(const BindingState& owner, BorrowMode mode, const std::string& place = {}) {
    if (owner.isMoved()) return false;
    for (const auto& existing : owner.activeBorrows) {
        if (!placesOverlap(existing.place, place)) continue;
        if (mode == BorrowMode::Exclusive || existing.mode == BorrowMode::Exclusive) return false;
    }
    return true;
}

inline bool mayMove(const BindingState& owner) {
    return !owner.isMoved() && !owner.hasAnyBorrow();
}

inline bool mayAccess(const BindingState& owner, BorrowMode mode, const std::string& place = {}) {
    if (owner.isMoved()) return false;
    for (const auto& existing : owner.activeBorrows) {
        if (!placesOverlap(existing.place, place)) continue;
        if (mode == BorrowMode::Exclusive || existing.mode == BorrowMode::Exclusive) return false;
    }
    return true;
}

inline void beginBorrow(BindingState& owner, BorrowRecord borrow) {
    if (!mayBeginBorrow(owner, borrow.mode, borrow.place)) {
        throw std::logic_error("invalid overlapping borrow");
    }
    owner.activeBorrows.push_back(std::move(borrow));
}

inline void endBorrow(BindingState& owner, const BorrowRecord& borrow) {
    const auto it = std::find_if(owner.activeBorrows.begin(), owner.activeBorrows.end(), [&](const BorrowRecord& existing) {
        return existing.mode == borrow.mode && existing.ownerId == borrow.ownerId &&
               existing.ownerBinding == borrow.ownerBinding && existing.place == borrow.place;
    });
    if (it != owner.activeBorrows.end()) owner.activeBorrows.erase(it);
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

} // namespace lanner::memory
