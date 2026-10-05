main() i32:
    comptime FLAG = true == false
    if FLAG:
        return 0
    return 1
