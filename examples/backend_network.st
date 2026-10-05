static mut Ready: Atomic[u64] = 0
static mut Result: Atomic[u64] = 0

receiver() void:
    socket: Socket = Net.udpOpen()
    bound: bool = Net.udpBind(socket, "127.0.0.1", 46872)
    if !bound:
        Result.store(1)
        Ready.store(2)
        Net.close(socket)
        return
    Net.setNonblocking(socket, true)
    Ready.store(1)
    poller: Poller = Poller.create()
    Poller.add(poller, socket, Poller.readEvents())
    if Poller.wait(poller, 2000) > 0 && (Poller.eventMask(poller, 0) & Poller.readEvents()) != 0:
        buffer: *mut u8 = null
        unsafe:
            buffer = stackAlloc(16, 8)
        count: isize = Net.udpRecv(socket, buffer, 1)
        if count == 1:
            unsafe:
                value: u8 = unalignedLoad(buffer)
                if value == 90:
                    Result.store(90)
    Poller.destroy(poller)
    Net.close(socket)

main() i32:
    worker: Thread = Thread.spawn(receiver)
    deadline: u64 = Clock.deadlineAfterNanos(2000000000)
    while Ready.load() == 0 && !Clock.expired(deadline):
        Clock.sleepNanos(1000000)
    if Ready.load() != 1:
        worker.join()
        return 1

    socket: Socket = Net.udpOpen()
    payload: *mut u8 = null
    unsafe:
        payload = stackAlloc(16, 8)
        unalignedStore(payload, 90)
    sent: isize = Net.udpSendTo(socket, "127.0.0.1", 46872, payload, 1)
    Net.close(socket)
    worker.join()
    if sent != 1 || Result.load() != 90:
        return 2

    child: Process = Process.spawn("echo stable-process")
    status: i32 = Process.wait(child)
    if status != 0:
        return 3
    return 42
