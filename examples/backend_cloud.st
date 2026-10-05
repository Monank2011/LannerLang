static mut Ready: Atomic[u64] = 0

server() void:
    listener: Socket = Net.tcpListen("127.0.0.1", 46871, 16)
    Ready.store(1)
    poller: Poller = Poller.create()
    Poller.add(poller, listener, Poller.readEvents())
    handled: i32 = 0
    while handled < 2:
        if Poller.wait(poller, 2000) > 0:
            client: Socket = Net.accept(listener)
            Net.setNonblocking(client, false)
            request: *mut u8 = null
            unsafe:
                request = stackAlloc(4096, 16)
            Net.recv(client, request, 4096)
            Net.sendString(client, "HTTP/1.1 200 OK\r\nContent-Length: 11\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\nbackend-ok\n")
            Net.close(client)
            handled = handled + 1
        else:
            break
    Poller.remove(poller, listener)
    Poller.destroy(poller)
    Net.close(listener)

main() i32:
    worker: Thread = Thread.spawn(server)
    deadline: u64 = Clock.deadlineAfterNanos(2000000000)
    while Ready.load() == 0 && !Clock.expired(deadline):
        Clock.sleepNanos(1000000)
    if Ready.load() == 0:
        worker.join()
        return 1

    response: Buffer = Http.get("http://127.0.0.1:46871/health", 2000)
    status: i32 = Http.status()
    goodGet: bool = status == 200 && response.cstr().startsWith("backend-ok")
    response.free()

    posted: Buffer = Http.post("http://127.0.0.1:46871/api", "{\"op\":\"ping\"}", 2000)
    postStatus: i32 = Http.status()
    goodPost: bool = postStatus == 200 && posted.cstr().startsWith("backend-ok")
    posted.free()
    worker.join()
    if !goodGet || !goodPost:
        return 3

    jsonOk: bool = Json.validate("{\"ok\":true,\"n\":42}")
    jsonBad: bool = Json.validate("{\"x\":\"\\q\"}")
    quoted: Buffer = Json.quote("stable")
    number: Buffer = Json.int(42)
    flag: Buffer = Json.bool(true)
    nul: Buffer = Json.nullValue()
    quotedOk: bool = quoted.cstr().equals("\"stable\"")
    numberOk: bool = number.cstr().equals("42")
    flagOk: bool = flag.cstr().equals("true")
    nullOk: bool = nul.cstr().equals("null")
    quoted.free()
    number.free()
    flag.free()
    nul.free()
    if !jsonOk || jsonBad || !quotedOk || !numberOk || !flagOk || !nullOk:
        return 4

    output: Buffer = Process.output("echo backend-ok")
    outputOk: bool = output.cstr().startsWith("backend-ok")
    output.free()
    if !outputOk:
        return 5

    envOk: bool = Process.setEnv("STABLE_BACKEND_TEST", "ok")
    env: string = getEnv("STABLE_BACKEND_TEST")
    if !envOk || !env.equals("ok"):
        return 6

    return 42
