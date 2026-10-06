main() i32:
    flags = Game.windowFlags(true, false, false, false)
    window = Game.createWindow("Lanner OpenGL", 96, 96, flags)
    if !Game.makeGLContext(window):
        Game.destroyWindow(window)
        return 1
    r:f32=0.1
    g:f32=0.2
    b:f32=0.3
    a:f32=1.0
    Graphics.glViewport(0, 0, 96, 96)
    Graphics.glClearColor(r, g, b, a)
    Graphics.glClear(16384)
    err = Graphics.glGetError()
    Game.present(window)
    Game.destroyWindow(window)
    if err != 0:
        return 2
    return 0
