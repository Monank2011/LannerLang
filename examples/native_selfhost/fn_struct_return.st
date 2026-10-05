Point[x: i32, y: i32]

make(a: i32, b: i32) Point:
    return Point[x: a, y: b]

main() i32:
    p = make(20, 22)
    return p.x + p.y
