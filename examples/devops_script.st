main() i32:
    if Args.count() < 3:
        return 1
    if !Args.at(1).equals("alpha"):
        return 2
    if !Args.at(2).equalsIgnoreCase("BETA"):
        return 3

    if !Env.set("LANNER_STEP7", "ok"):
        return 4
    if !Env.has("LANNER_STEP7"):
        return 5
    if !Env.get("LANNER_STEP7").equals("ok"):
        return 6

    root: string = FS.cwd()
    path: string = Path.join(root, "step7-devops.txt")
    copyPath: string = Path.join(root, "step7-devops.copy.txt")
    movedPath: string = Path.join(root, "step7-devops.moved.txt")

    FS.remove(path)
    FS.remove(copyPath)
    FS.remove(movedPath)

    if !FS.write(path, "Lanner script 42"):
        return 7
    if !FS.exists(path) || !FS.isFile(path):
        return 8
    if FS.fileSize(path) <= 0:
        return 9

    data: Buffer = FS.read(path)
    if !data.cstr().startsWith("Lanner"):
        return 10
    if data.cstr().find("42") < 0:
        return 11
    data.free()

    if !FS.copy(path, copyPath):
        return 12
    if !FS.rename(copyPath, movedPath):
        return 13
    if !FS.exists(movedPath):
        return 14

    pattern: Regex = Regex.compile("^[A-Za-z]+ [a-zA-Z0-9]+$")
    if !Regex.isMatch(pattern, "Lanner 42"):
        return 15
    Regex.free(pattern)

    n: i64 = String.parseInt("42")
    f: f64 = String.parseFloat("3.5")
    if n != 42:
        return 16
    if f < 3.49 || f > 3.51:
        return 17

    out: Buffer = Shell.output("echo lanner-script")
    if !out.cstr().contains("lanner-script"):
        return 18
    out.free()
    if Shell.run("exit 0") != 0:
        return 19
    if Shell.which("cmake").len() == 0:
        return 20

    listing: Buffer = FS.list(root)
    if listing.len() == 0:
        return 21
    listing.free()

    FS.remove(path)
    FS.remove(movedPath)
    if FS.exists(path) || FS.exists(movedPath):
        return 22
    if !Env.unset("LANNER_STEP7"):
        return 23
    return 42
