addOne(xs: &mut []i64) void:
    xs.push(42)

main() i32:
    xs = []i64
    addOne(&mut xs)
    addOne(&mut xs)
    t: i64 = xs[0] + xs[1]
    return t as i32
