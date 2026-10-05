main() i32:
    windowFlags = Game.windowFlags(false, true, false, false)
    window = Game.createWindow("Stable Game", 320, 240, windowFlags)
    if Game.width(window) != 320:
        Game.destroyWindow(window)
        return 1
    rendererFlags = Game.rendererFlags(true, false)
    renderer = Game.createRenderer(window, rendererFlags)
    frame = 0
    while frame < 3:
        Game.poll(window)
        eventType = Game.eventType()
        eventCode = Game.eventCode()
        eventX = Game.eventX()
        eventY = Game.eventY()
        eventText = Game.eventText()
        keyState = Game.keyDown(4)
        mouseState = Game.mouseButtonDown(1)
        mouseX = Game.mouseX()
        mouseY = Game.mouseY()
        pad = Game.controllerConnected(0)
        axis = Game.controllerAxis(0, 0)
        padButton = Game.controllerButtonDown(0, 0)
        Game.setDrawColor(renderer, 18, 28, 48, 255)
        Game.clear(renderer)
        Game.setDrawColor(renderer, 220, 80, 60, 255)
        Game.fillRect(renderer, 20 + frame * 10, 30, 80, 50)
        Game.drawLine(renderer, 0, 0, 319, 239)
        Game.presentRenderer(renderer)
        Game.deltaSeconds()
        Game.sleepNanos(1000000)
        frame = frame + 1
    audio = Game.audioOpen(8000, 1, 256)
    if Game.audioQueued(audio) >= 0:
        unsafe:
            audioData:*mut u8=allocAligned(256,16)
            memset(audioData,0,256)
            written:isize=Game.audioWrite(audio,audioData,256)
            deallocAligned(audioData)
            if written != 256:
                Game.audioClose(audio)
                Game.destroyRenderer(renderer)
                Game.destroyWindow(window)
                return 3
        Game.audioPause(audio, true)
        Game.audioClose(audio)
    Game.destroyRenderer(renderer)
    Game.requestClose(window)
    if !Game.shouldClose(window):
        Game.destroyWindow(window)
        return 2
    Game.destroyWindow(window)
    return 0
