main() i32:
    backend = Graphics.backend()
    if backend.len() == 0:
        return 1
    if !Graphics.available("OpenGL"):
        return 2
    unsafe:
        proc:*mut u8=Graphics.loadProc("OpenGL", "glGetError")
        if proc==null:
            return 3
    return 0
