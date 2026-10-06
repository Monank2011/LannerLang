main() i32:
    start: u64 = Clock.monotonicNanos()
    deadline: u64 = Clock.deadlineAfterNanos(100000000)
    remaining: u64 = Clock.remainingNanos(deadline)
    elapsed: u64 = Clock.monotonicNanos() - start
    alive: bool = !Clock.expired(deadline)
    return ((remaining > 0 && alive && elapsed < 100000000) as i32) * 42
