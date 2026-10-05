clamp(a: i32) i32:
    x: i32 = a
    x = x + 1
    while x < 5:
        x = x + 1
    if x >= 5:
        return x as i32
    return 0

main() i32:
    value: i32 = clamp(1)
    if value <= 5:
        return 0
    return 1
