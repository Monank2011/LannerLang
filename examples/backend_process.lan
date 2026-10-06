main() i32:
    child: Process = Process.spawn("true")
    pid: u64 = Process.pid(child)
    status: i32 = Process.wait(child)
    if pid == 0 || status != 0:
        return 1

    output: Buffer = Process.output("printf lanner-output")
    good: bool = output.cstr().equals("lanner-output")
    output.free()
    if !good:
        return 2
    return 42
