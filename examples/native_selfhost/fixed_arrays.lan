Point[x:i32, y:i32]
Box[vals:[2]i32]

mutate(a: &mut [3]i32) void:
    a[0] = 9
    a[2] = 13

pack(a: [3]i32) [3]i32:
    a[1] = 20
    return a

sum(a: [3]i32) i32:
    return a[0] + a[1] + a[2]

main() i32:
    a: [3]i32 = [10, 2, 12]
    mutate(&mut a)
    b: [3]i32 = pack(a)
    p: [2]Point = [Point[x: 1, y: 2], Point[x: 3, y: 4]]
    p[1].x = 9
    box = Box[vals: [5, 6]]
    return sum(b) + p[0].x + p[0].y + p[1].x + p[1].y + box.vals[0] + box.vals[1]
