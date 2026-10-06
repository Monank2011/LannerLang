static mut Nodes: Atomic[u64] = 0

worker() void:
    Nodes.fetchAdd(41)

main() i32:
    thread: Thread = Thread.spawn(worker)
    thread.join()
    return Nodes.load() as i32
