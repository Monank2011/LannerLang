
static mut Nodes: Atomic[u64] = 0
static mut StopFlag: Atomic[u64] = 0
static mut Active: Atomic[u64] = 0
static TABLE: [8]u64 = [1, 2, 4, 8, 16, 32, 64, 128]
static mut Deadline: Atomic[u64] = 0

searchWorker() void:
    while StopFlag.load() == 0 && !Clock.expired(Deadline.load()):
        Nodes.fetchAdd(1)
        Thread.yield()
    Active.store(0)

main() i32:
    running: bool = true
    writeRaw("id name Lanner\nid author Lanner\n")

    while running:
        line: string = Stdin.readLine()

        if line.equals("uci"):
            writeRaw("uciok\n")
        else:
            if line.equals("isready"):
                writeRaw("readyok\n")
            else:
                if line.equals("ucinewgame"):
                    Nodes.store(0)
                else:
                    if line.startsWith("go"):
                        budgetMs: u64 = line.parseU64At(3)
                        if budgetMs == 0:
                            budgetMs = 1
                        StopFlag.store(0)
                        Active.store(1)
                        Deadline.store(Clock.deadlineAfterNanos(budgetMs * 1000000))
                        worker: Thread = Thread.spawn(searchWorker)

                        while Active.load() != 0:
                            if Stdin.hasInput():
                                command: string = Stdin.readLine()
                                if command.equals("stop"):
                                    StopFlag.store(1)
                                else:
                                    if command.equals("quit"):
                                        StopFlag.store(1)
                                        running = false
                            else:
                                Thread.yield()

                        worker.join()
                        writeRaw("bestmove 0000\n")
                    else:
                        if line.equals("stop"):
                            StopFlag.store(1)
                        else:
                            if line.equals("quit"):
                                running = false

    return 0
