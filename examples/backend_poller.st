main() i32:
    poller: Poller = Poller.create()
    sockets: []Socket = []Socket
    i: i32 = 0
    while i < 128:
        socket: Socket = Net.udpOpen()
        if !Poller.add(poller, socket, Poller.readEvents()):
            Poller.destroy(poller)
            return 1
        sockets.push(socket)
        i = i + 1
    ready: i32 = Poller.wait(poller, 10)
    if ready != 0 || Poller.count(poller) != 0:
        i = 0
        while i < 128:
            Net.close(sockets[i])
            i = i + 1
        Poller.destroy(poller)
        return 2
    i = 0
    while i < 128:
        Poller.remove(poller, sockets[i])
        Net.close(sockets[i])
        i = i + 1
    Poller.destroy(poller)
    return 42
